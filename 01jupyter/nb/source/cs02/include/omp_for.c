#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
int main(int argc, char ** argv) {
  char * nthreads_ = getenv("OMP_NUM_THREADS");
  int    nthreads  = (nthreads_ ? atoi(nthreads_) : 1);
  if (nthreads % 32) {
    fprintf(stderr, "OMP_NUM_THREADS (%d) must be a multiple of 32\n", nthreads);
    exit(1);
  }
  int m = (argc > 1 ? atoi(argv[1]) : 5);
  int n = (argc > 2 ? atoi(argv[2]) : 6);
  printf("hello on host\n");
#pragma omp target teams
  {
    printf("in teams: %03d/%03d\n", omp_get_team_num(), omp_get_num_teams());
#pragma omp distribute
    for (int i = 0; i < m; i++) {
      printf("in distribute: i=%03d executed by %03d/%03d\n",
             i, omp_get_team_num(), omp_get_num_teams());
#pragma omp parallel num_threads(nthreads)
      printf("in parallel: i=%03d %03d/%03d %03d/%03d\n",
             i, omp_get_team_num(), omp_get_num_teams(),
             omp_get_thread_num(), omp_get_num_threads());
#pragma omp for
      for (int j = 0; j < n; j++) {
        printf("in for: i=%03d j=%03d executed by %03d/%03d %03d/%03d\n",
               i, j, omp_get_team_num(), omp_get_num_teams(),
               omp_get_thread_num(), omp_get_num_threads());
      }
    }
  }
  printf("back on host\n");
  return 0;
}
