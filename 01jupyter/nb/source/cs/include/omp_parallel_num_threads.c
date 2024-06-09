#include <stdio.h>
#include <stdlib.h>

int main(int argc, char ** argv) {
  int nt = atoi(argv[1]);
  printf("before parallel\n");
#pragma omp parallel num_threads(nt)
  printf("in parallel\n");
  printf("after parallel\n");
  return 0;
}

