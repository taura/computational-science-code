#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <omp.h>
#include <time.h>
#include <unistd.h>

/* x = ax + b をひたすら n 回繰り返す.
   (|a| < 1.0 なら c によらず, x = b / (1 - a) に収束).
   n 回 mul + add を行う (-> 2 n flops) */
double lin_rec(double a, double b, double c, long n) {
  double x = c;
  for (long j = 0; j < n; j++) {
    x = a * x + b;
  }
  return x;
}

int main(int argc, char ** argv) {
  char * nteams_   = getenv("OMP_NUM_TEAMS");
  int    nteams    = (nteams_   ? atoi(nteams_) : 1);
  char * nthreads_ = getenv("OMP_NUM_THREADS");
  int    nthreads  = (nthreads_ ? atoi(nthreads_) : 1);
  long m           = (1 < argc ? atol(argv[1]) : nteams * nthreads);
  long n           = (2 < argc ? atol(argv[2]) : 100 * 1000 * 1000);
  double * x = (double *)calloc(sizeof(double), m);
  assert(x);
  printf("num_teams = %ld, num_threads = %ld\n", nteams, nthreads);
  printf("m = %ld, n = %ld\n", m, n);
  /* 計測開始 */
  double t0 = omp_get_wtime();
  /* 計算本体 */
#pragma omp target teams distribute parallel for num_teams(nteams) num_threads(nthreads) map(tofrom: x[0:m])
  for (long i = 0; i < m; i++) {
    x[i] = lin_rec(0.99, i + 1, 1.0, n);
  }
  /* 計測終了 */
  double t1 = omp_get_wtime();
  double dt = t1 - t0;          /* sec */
  
  /* 答え表示 (x[i] = 100 * (i + 1) くらいのはず) */
  long err = 0;
  for (long i = 0; i < m; i++) {
    if (fabs(x[i] - 100 * (i + 1)) > 1.0e-3) {
      printf("x[%3ld] = %9.3f\n", i, x[i]);
      err++;
    }
  }
  if (err == 0) {
    printf("OK\n");
  }
  double flops = 2. * (double)m * (double)n;
  printf("elapsed    : %7.3f  sec\n", dt);
  printf("elapsed/m  : %7.3f msec\n", dt / m * 1e3);
  printf("elapsed/n  : %7.3f nsec\n", dt / n * 1e9);
  printf("elapsed/mn : %7.3f nsec\n", dt / (m * n) * 1e9);
  printf("flops      : %.2e\n", flops);
  printf("%.3f GFLOPS\n", flops / dt * 1e-9);
  return 0;
}

