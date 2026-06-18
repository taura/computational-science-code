#include <cstdio>
#include <cstdlib>
#include <omp.h>

int main(int argc, char ** argv) {
  int m = (argc > 1 ? atoi(argv[1]) : 5);
  printf("hello on host\n");
#pragma omp target teams distribute
  for (int i = 0; i < m; i++) {
    printf("in distribute: i=%03d executed by %03d/%03d\n",
           i, omp_get_team_num(), omp_get_num_teams());
  }
  printf("back on host\n");
  return 0;
}
