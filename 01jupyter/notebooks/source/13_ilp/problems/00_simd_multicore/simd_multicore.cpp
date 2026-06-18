#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <cmath>
#include <omp.h>

/* ベクトル長 nl. 2 のべき乗 (8, 16, 32, ...) に変えて性能を比べてみよ */
enum { nl = 8 };

typedef double doublev __attribute__((vector_size(sizeof(double) * nl),
                                      aligned(sizeof(double))));

/* スカラ u を全要素 u のベクトルに */
doublev uniform(double u) {
  doublev v;
  for (long i = 0; i < nl; i++) {
    v[i] = u;
  }
  return v;
}

/* u, u+1, u+2, ... からなるベクトル */
doublev range(double u) {
  doublev v;
  for (long i = 0; i < nl; i++) {
    v[i] = u + i;
  }
  return v;
}

/* ベクトル型 v を配列 a の先頭 nl 要素に書き込む */
void storev(double * a, doublev v) {
  for (int i = 0; i < nl; i++) {
    a[i] = v[i];
  }
}

/* b, t をベクトル型にした lin_rec.
   nl 個の独立な漸化式を同時に進める */
doublev lin_rec(double a, doublev b, double c, long n) {
  doublev t = uniform(c);
  for (long j = 0; j < n; j++) {
    t = a * t + b;
  }
  return t;
}

int main(int argc, char ** argv) {
  long m     = (1 < argc ? atol(argv[1]) : 8);
  long n     = (2 < argc ? atol(argv[2]) : 1000 * 1000 * 1000);
  double * x = (double *)calloc(sizeof(double), m);
  assert(x);
  printf("m = %ld, n = %ld, nl = %d\n", m, n, nl);
  /* 計測開始 */
  double t0 = omp_get_wtime();
  /* 計算本体 (nl 回ずつまとめて SIMD 実行). */
  // TODO: 下の for ループの直前に #pragma omp parallel for を1行追加し, 外側のループ (互いに独立) をマルチコアで並列化せよ.
  for (long i = 0; i < m; i += nl) {
    storev(&x[i], lin_rec(0.99, range(i) + 1, 1.0, n));
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
  printf("flops      : %.2e\n", flops);
  printf("%.3f GFLOPS\n", flops / dt * 1e-9);
  return 0;
}
