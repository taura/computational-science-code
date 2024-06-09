#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <unistd.h>

int main(int argc, char ** argv) {
  int m = (1 < argc ? atoi(argv[1]) : 10);
  printf("before parallel\n");
#pragma omp parallel
  {
    int nt = omp_get_num_threads();
#pragma omp for schedule(static)
    for (int i = 0; i < m; i++) {
      int t = omp_get_thread_num();
      usleep(i * 100000);
      printf("i = %3d, by thread %3d of %3d\n", i, t, nt);
    }
  }
  printf("after parallel\n");
  return 0;
}

