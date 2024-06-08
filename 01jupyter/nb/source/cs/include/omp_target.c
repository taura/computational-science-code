#include <stdio.h>

int main() {
  printf("before target\n");
#pragma omp target teams num_teams(3)
  {
    printf("in target\n");
  }
  printf("after target\n");
  return 0;
}
