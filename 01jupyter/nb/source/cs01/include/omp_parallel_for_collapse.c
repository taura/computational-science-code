#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <unistd.h>

int main(int argc, char ** argv) {
  int m = (1 < argc ? atoi(argv[1]) : 4);
#pragma omp parallel
  {
    int nt = omp_get_num_threads();
#pragma omp for collapse(2)
    for (int i = 0; i < m; i++) {
      for (int j = 0; j < m; j++) {
        int t = omp_get_thread_num();
        usleep((i + j) * 100000);
        printf("i,j = %3d,%3d, by thread %3d of %3d\n", i, j, t, nt);
      }
    }
  }
  return 0;
}

