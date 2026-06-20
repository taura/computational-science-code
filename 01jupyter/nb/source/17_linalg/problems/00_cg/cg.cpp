#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <omp.h>

/* 状態を持たない乱数 (既知解の生成用): (seed,k) から [0,1)。 */
static inline double draw_rand01(long long seed, long long k) {
  const long long M = 2147483647LL;
  long long x = ((seed % M) * 2654435761LL + (k % M) + 1) % M;
  x = ((x ^ (x >> 16)) * 1812433253LL) % M;
  x = ((x ^ (x >> 13)) * 1664525LL)    % M;
  x =  (x ^ (x >> 16)) % M;
  return (double)x / (double)M;
}

/* 行列ベクトル積 y = A p。
   A は n×n 格子上の 2次元ラプラシアン (5点ステンシル, ディリクレ境界=0) で,
   対称正定値 (SPD)。行列を保持せず, ステンシルの計算で済ませる (行列フリー)。
   (A p)[i,j] = 4 p[i,j] - p[i-1,j] - p[i+1,j] - p[i,j-1] - p[i,j+1]  (領域外は 0)。
   これは B1 (2D熱伝導) と同じラプラシアン行列である。 */
void matvec(int n, const double * p, double * y) {
  // BEGIN ANSWER: 格子点の二重ループを #pragma omp parallel for collapse(2) で並列化せよ.
#pragma omp parallel for collapse(2)
  // END ANSWER
  for (int i = 0; i < n; i++) {
    for (int j = 0; j < n; j++) {
      double v = 4.0 * p[i*n+j];
      if (i > 0)     v -= p[(i-1)*n+j];
      if (i < n - 1) v -= p[(i+1)*n+j];
      if (j > 0)     v -= p[i*n+j-1];
      if (j < n - 1) v -= p[i*n+j+1];
      y[i*n+j] = v;
    }
  }
}

/* 内積 a・b */
double dot(long N, const double * a, const double * b) {
  double s = 0.0;
  // BEGIN ANSWER: 内積の和を #pragma omp parallel for reduction(+:s) で並列化せよ.
#pragma omp parallel for reduction(+:s)
  // END ANSWER
  for (long k = 0; k < N; k++) s += a[k] * b[k];
  return s;
}

int main(int argc, char ** argv) {
  int    n   = (argc > 1 ? atoi(argv[1]) : 128);    /* 格子の一辺 (未知数は N = n*n) */
  double tol = (argc > 2 ? atof(argv[2]) : 1e-8);
  long   N   = (long)n * n;
  int    maxit = 10 * n;

  double * xt = (double *)malloc(sizeof(double) * N);  /* 既知の真の解 */
  double * b  = (double *)malloc(sizeof(double) * N);
  double * x  = (double *)malloc(sizeof(double) * N);
  double * r  = (double *)malloc(sizeof(double) * N);
  double * p  = (double *)malloc(sizeof(double) * N);
  double * Ap = (double *)malloc(sizeof(double) * N);

  for (long k = 0; k < N; k++) xt[k] = draw_rand01(k, 0);  /* 真の解をランダムに決め */
  matvec(n, xt, b);                                        /* b = A xt を作る */

  /* CG: x=0 から始めて A x = b を解く */
  for (long k = 0; k < N; k++) { x[k] = 0.0; r[k] = b[k]; p[k] = b[k]; }
  double rs = dot(N, r, r);

  int it;
  double t0 = omp_get_wtime();
  for (it = 0; it < maxit; it++) {
    matvec(n, p, Ap);
    double alpha = rs / dot(N, p, Ap);
    for (long k = 0; k < N; k++) { x[k] += alpha * p[k]; r[k] -= alpha * Ap[k]; }  /* (発展: ここも並列化可) */
    double rs_new = dot(N, r, r);
    if (sqrt(rs_new) < tol) { rs = rs_new; it++; break; }
    double beta = rs_new / rs;
    for (long k = 0; k < N; k++) p[k] = r[k] + beta * p[k];
    rs = rs_new;
  }
  double elapsed = omp_get_wtime() - t0;

  /* 検算: 求めた x が真の解 xt にどれだけ近いか */
  double err = 0.0;
  for (long k = 0; k < N; k++) { double e = fabs(x[k] - xt[k]); if (e > err) err = e; }
  printf("n=%d (N=%ld), iters=%d, 残差=%.2e, 解の誤差(max|x-xt|)=%.2e\n",
         n, N, it, sqrt(rs), err);
  printf("elapsed = %.3f sec\n", elapsed);
  free(xt); free(b); free(x); free(r); free(p); free(Ap);
  return 0;
}
