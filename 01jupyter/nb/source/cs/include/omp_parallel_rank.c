#include <stdio.h>
#include <omp.h>

int main() {
  printf("hello\n");
#pragma omp parallel
  {
    int omp_nthreads = omp_get_num_threads();
    int omp_rank = omp_get_thread_num();
    printf("world %d/%d\n", omp_rank, omp_nthreads);
  }
  printf("good bye\n");
  return 0;
}
