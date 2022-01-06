#define def_N 15728640
#define def_M 16

kernel void benchmark_1(global float* data) {
    const uint n = get_global_id(0);
    #pragma unroll
    for(uint i=0; i<def_M; i++) data[i*def_N+n] = 0.0f; // M coalesced writes
}

kernel void benchmark_2(global float* data) {
    const uint n = get_global_id(0);
    float x = 0.0f;
    #pragma unroll
    for(uint i=0; i<def_M; i++) x += data[i*def_N+n]; // M coalesced reads
    data[n] = x; // 1 coalesced write (to prevent compiler optimization)
}

kernel void benchmark_3(global float* data) {
    const uint n = get_global_id(0);
    #pragma unroll
    for(uint i=0; i<def_M; i++) data[n*def_M+i] = 0.0f; // M misaligned writes
}

kernel void benchmark_4(global float* data) {
    const uint n = get_global_id(0);
    float x = 0.0f;
    #pragma unroll
    for(uint i=0; i<def_M; i++) x += data[n*def_M+i]; // M misaligned reads
    data[n] = x; // 1 coalesced write (to prevent compiler optimization)
}
