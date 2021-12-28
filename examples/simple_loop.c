int simple_loop(int *A, int *B, int size){
    for(int i=0; i<size; i++){
        A[i] = B[i] + 42;
    }    
    return A[size-1];
} 