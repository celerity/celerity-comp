#define NBRLIST_DIM  11
#define NBRLIST_MAXLEN (NBRLIST_DIM * NBRLIST_DIM * NBRLIST_DIM)

#define BIN_DEPTH         8  /* max number of atoms per bin */
#define BIN_SIZE         32  /* size of bin in floats */
#define BIN_SHIFT         5  /* # of bits to shift for mul/div by BIN_SIZE */
#define BIN_CACHE_MAXLEN 32  /* max number of atom bins to cache */

#define BIN_LENGTH      4.f  /* spatial length in Angstroms */
#define BIN_INVLEN  (1.f / BIN_LENGTH)
/* assuming density of 1 atom / 10 A^3, expectation is 6.4 atoms per bin
 * so that bin fill should be 80% (for non-empty regions of space) */

#define REGION_SIZE     512  /* number of floats in lattice region */

#define SUB_REGION_SIZE 256

// OpenCL 1.1 support for int3 is not uniform on all implementations, so
// we use int4 instead.  Only the 'x', 'y', and 'z' fields of xyz are used.
typedef int4 xyz;


__kernel void opencl_cutoff_potential_lattice(
    int binDim_x,
    int binDim_y,
    __global float4 *binBaseAddr,
    int offset,
    float h,                /* lattice spacing */
    float cutoff2,          /* square of cutoff distance */
    float inv_cutoff2,
    __global float *regionZeroAddr,  /* address of lattice regions starting at origin */
    int zRegionIndex,
    __constant int *NbrListLen,
    __constant xyz *NbrList
    )
{
  __global float4* binZeroAddr = binBaseAddr + offset;

  __local float AtomBinCache[BIN_CACHE_MAXLEN * BIN_DEPTH * 4];
  __global float *mySubRegionAddr;
  __local xyz myBinIndex;

  /* thread id */
  const int tid = (get_local_id(2)*8 + get_local_id(1))*8 + get_local_id(0);

  /* neighbor index */
  int nbrid;

  /* this is the start of the sub-region indexed by tid */
  mySubRegionAddr = regionZeroAddr + ((zRegionIndex*get_num_groups(1)
	+ get_group_id(1))*(get_num_groups(0)>>2) + (get_group_id(0)>>2))*REGION_SIZE
	+ (get_group_id(0)&3)*SUB_REGION_SIZE;

  /* spatial coordinate of this lattice point */
  float x = (8 * (get_group_id(0) >> 2) + get_local_id(0)) * h;
  float y = (8 * get_group_id(1) + get_local_id(1)) * h;
  float z = (8 * zRegionIndex + 2*(get_group_id(0)&3) + get_local_id(2)) * h;

  int totalbins = 0;
  int numbins;

  /* bin number determined by center of region */
  myBinIndex.x = (int) floor((8 * (get_group_id(0) >> 2) + 4) * h * BIN_INVLEN);
  myBinIndex.y = (int) floor((8 * get_group_id(1) + 4) * h * BIN_INVLEN);
  myBinIndex.z = (int) floor((8 * zRegionIndex + 4) * h * BIN_INVLEN);

  /* first neighbor in list for me to cache */
  nbrid = (tid >> 4);

  numbins = BIN_CACHE_MAXLEN;

  float energy = 0.f;
  for (totalbins = 0;  totalbins < *NbrListLen;  totalbins += numbins) {
    int bincnt;

    /* start of where to write in shared memory */
    int startoff = BIN_SIZE * (tid >> 4);

    /* each half-warp to cache up to 4 atom bins */
    for (bincnt = 0;  bincnt < 4 && nbrid < *NbrListLen;  bincnt++, nbrid += 8) {
      int i = myBinIndex.x + NbrList[nbrid].x;
      int j = myBinIndex.y + NbrList[nbrid].y;
      int k = myBinIndex.z + NbrList[nbrid].z;

      /* determine global memory location of atom bin */
      __global float *p_global = ((__global float *) binZeroAddr)
       + (((k*binDim_y) + j)*binDim_x + i) * BIN_SIZE;

      /* coalesced read from global memory -
       * retain same ordering in shared memory for now */
      int tidmask = tid & 15;
      int binIndex = startoff + bincnt*8*BIN_SIZE;

      AtomBinCache[binIndex + tidmask   ] = p_global[tidmask   ];
      AtomBinCache[binIndex + tidmask+16] = p_global[tidmask+16];
    }
    barrier(CLK_LOCAL_MEM_FENCE|CLK_GLOBAL_MEM_FENCE);    

    /* no warp divergence */
    if (totalbins + BIN_CACHE_MAXLEN > *NbrListLen) {
      numbins = *NbrListLen - totalbins;
    }

    for (bincnt = 0;  bincnt < numbins;  bincnt++) {
      int i;
      float r2;

      for (i = 0;  i < BIN_DEPTH;  i++) {
        float ax = AtomBinCache[bincnt * BIN_SIZE + i*4];
        float ay = AtomBinCache[bincnt * BIN_SIZE + i*4 + 1];
        float az = AtomBinCache[bincnt * BIN_SIZE + i*4 + 2];
        float aq = AtomBinCache[bincnt * BIN_SIZE + i*4 + 3];
        if (0.f == aq) break;  /* no more atoms in bin */
        r2 = (ax - x) * (ax - x) + (ay - y) * (ay - y) + (az - z) * (az - z);
        if (r2 < cutoff2) {
          float s = (1.f - r2 * inv_cutoff2);
          energy += aq * rsqrt(r2) * s * s;
        }
      } /* end loop over atoms in bin */
    } /* end loop over cached atom bins */
    barrier(CLK_LOCAL_MEM_FENCE|CLK_GLOBAL_MEM_FENCE);

  } /* end loop over neighbor list */

  /* store into global memory */
  mySubRegionAddr[tid] = energy;
}

/////////////////////////////////////////////////////////////////////////////////
__kernel void convolute(
  __global float * output,
  const __global float * input,
  const __global float * filter,
  int HALF_FILTER_SIZE,
  int IMAGE_H,
  int IMAGE_W
)
{

  int row = get_global_id(1);
  int col = get_global_id(0);
  int idx = col + row * IMAGE_W;

  if (
    col < HALF_FILTER_SIZE ||
    col > IMAGE_W - HALF_FILTER_SIZE - 1 ||
    row < HALF_FILTER_SIZE ||
    row > IMAGE_H - HALF_FILTER_SIZE - 1
  ) {
    if (row < IMAGE_W && col < IMAGE_H) {
      output[idx] = 0.0f;
    }
  } else {
    // perform convolution
    int fIndex = 0;
    float result = 0.0f;

    for (int r = -HALF_FILTER_SIZE; r <= HALF_FILTER_SIZE; r++) {
      for (int c = -HALF_FILTER_SIZE; c <= HALF_FILTER_SIZE; c++) {
        int offset = c + r * IMAGE_W;
        result += input[ idx + offset ] * filter[fIndex];
        fIndex++;
      }
    }
    output[idx] = result;
  }
}