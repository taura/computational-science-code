#include <cstdio>

int main() {
  printf("before parallel\n");
#pragma omp parallel
  printf("in parallel\n");
  printf("after parallel\n");
  return 0;
}
