#include <stdio.h>
#include <stdlib.h>
#include <omp.h>

int main(int argc, char ** argv) {
  int m = (1 < argc ? atoi(argv[1]) : 10);
  printf("before parallel\n");
#pragma omp parallel
  {
    printf("in parallel, before for\n");
#pragma omp for
    for (int i = 0; i < m; i++) {
      printf("i = %3d\n", i);
    }
    printf("in parallel, after for\n");
  }
  printf("after parallel\n");
  return 0;
}

