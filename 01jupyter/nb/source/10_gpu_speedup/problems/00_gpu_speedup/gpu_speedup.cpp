#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <omp.h>

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
  /* チーム数・スレッド数を環境変数から取得 */
  char * nteams_   = getenv("OMP_NUM_TEAMS");
  int    nteams    = (nteams_   ? atoi(nteams_)   : 1);
  char * nthreads_ = getenv("OMP_NUM_THREADS");
  int    nthreads  = (nthreads_ ? atoi(nthreads_) : 1);
  long m = (1 < argc ? atol(argv[1]) : (long)nteams * nthreads);
  long n = (2 < argc ? atol(argv[2]) : 100 * 1000 * 1000);
  double * x = (double *)calloc(sizeof(double), m);
  assert(x);
  printf("num_teams = %d, num_threads = %d\n", nteams, nthreads);
  printf("m = %ld, n = %ld\n", m, n);
  /* 計測開始 */
  double t0 = omp_get_wtime();
  /* 計算本体. 現状では指示行が無いのでCPU上で逐次に実行される. */
  // BEGIN ANSWER: 下の for 文の直前に #pragma omp target teams distribute parallel for num_teams(nteams) num_threads(nthreads) map(tofrom: x[0:m]) を1行追加し, ループをGPU上で並列実行させよ. (結果 x をCPUに戻して検算するので map(tofrom: x[0:m]) が必要)
#pragma omp target teams distribute parallel for num_teams(nteams) num_threads(nthreads) map(tofrom: x[0:m])
  // END ANSWER
  for (long i = 0; i < m; i++) {
    x[i] = lin_rec(0.99, i + 1, 1.0, n);
  }
  /* 計測終了 */
  double t1 = omp_get_wtime();
  double dt = t1 - t0;          /* sec */
  /* 答え確認 (x[i] = 100 * (i + 1) くらいのはず) */
  long err = 0;
  for (long i = 0; i < m; i++) {
    if (fabs(x[i] - 100 * (i + 1)) > 1.0e-3) {
      printf("x[%3ld] = %9.3f\n", i, x[i]);
      err++;
    }
  }
  if (err == 0) printf("OK\n");
  double flops = 2. * (double)m * (double)n;
  printf("elapsed    : %7.3f  sec\n", dt);
  printf("elapsed/m  : %7.3f msec\n", dt / m * 1e3);
  printf("elapsed/n  : %7.3f nsec\n", dt / n * 1e9);
  printf("elapsed/mn : %7.3f nsec\n", dt / (m * n) * 1e9);
  printf("flops      : %.2e\n", flops);
  printf("%.3f GFLOPS\n", flops / dt * 1e-9);
  return 0;
}
