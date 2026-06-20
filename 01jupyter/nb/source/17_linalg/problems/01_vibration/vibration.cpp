#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <omp.h>

/* 状態を持たない乱数 (初期ベクトルの生成用): (seed,k) から [0,1)。 */
static inline double draw_rand01(long long seed, long long k) {
  const long long M = 2147483647LL;
  long long x = ((seed % M) * 2654435761LL + (k % M) + 1) % M;
  x = ((x ^ (x >> 16)) * 1812433253LL) % M;
  x = ((x ^ (x >> 13)) * 1664525LL)    % M;
  x =  (x ^ (x >> 16)) % M;
  return (double)x / (double)M;
}

/* 行列ベクトル積 y = A p。
   A は n×n 格子上の 2次元ラプラシアン (5点ステンシル, ディリクレ境界=0)。
   (A p)[i,j] = 4 p[i,j] - p[i-1,j] - p[i+1,j] - p[i,j-1] - p[i,j+1]  (領域外は 0)。
   これは B1 (2D熱伝導) や G1 (CG) と同じラプラシアン行列である。 */
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
  int    n     = (argc > 1 ? atoi(argv[1]) : 128);   /* 格子の一辺 (未知数は N = n*n) */
  double tol   = (argc > 2 ? atof(argv[2]) : 1e-9);
  int    maxit = (argc > 3 ? atoi(argv[3]) : 100000);
  long   N     = (long)n * n;
  double sigma = 8.0;                                /* シフト量 (> lambda_max ≈ 8) */

  double * x  = (double *)malloc(sizeof(double) * N);
  double * y  = (double *)malloc(sizeof(double) * N);
  double * Ax = (double *)malloc(sizeof(double) * N);

  /* 初期ベクトル: すべて 1 から始めて正規化 */
  for (long k = 0; k < N; k++) x[k] = 1.0;
  double nrm0 = sqrt(dot(N, x, x));
  for (long k = 0; k < N; k++) x[k] /= nrm0;

  /* べき乗法: B = sigma*I - A を繰り返し掛ける。
     B の最大固有値 = sigma - lambda_min(A) に収束し, 固有ベクトルは基本振動モード。 */
  int it;
  double lamB = 0.0, lamB_prev = 0.0;
  double t0 = omp_get_wtime();
  for (it = 0; it < maxit; it++) {
    matvec(n, x, Ax);
    for (long k = 0; k < N; k++) y[k] = sigma * x[k] - Ax[k];   /* y = B x (逐次のまま) */
    lamB = dot(N, x, y) / dot(N, x, x);                          /* レイリー商 */
    double nrm = sqrt(dot(N, y, y));
    for (long k = 0; k < N; k++) x[k] = y[k] / nrm;              /* 正規化 (逐次のまま) */
    if (it > 0 && fabs(lamB - lamB_prev) < tol) { it++; break; }
    lamB_prev = lamB;
  }
  double elapsed = omp_get_wtime() - t0;

  double lambda_min = sigma - lamB;
  double analytic   = 4.0 - 4.0 * cos(M_PI / (n + 1));
  double rel_err    = fabs(lambda_min - analytic) / analytic;

  /* 固有ベクトルの検算: 解析解 v[i,j] = sin(pi(i+1)/(n+1)) sin(pi(j+1)/(n+1)) と比べる。
     まず符号を合わせ, 正規化した両者の相対 L2 誤差を計算する。 */
  double * ve = (double *)malloc(sizeof(double) * N);
  double vn = 0.0;
  for (int i = 0; i < n; i++)
    for (int j = 0; j < n; j++) {
      double v = sin(M_PI * (i+1) / (n+1)) * sin(M_PI * (j+1) / (n+1));
      ve[i*n+j] = v; vn += v * v;
    }
  vn = sqrt(vn);
  double sgn = (x[(n/2)*n + n/2] >= 0.0) ? 1.0 : -1.0;   /* 中央の値で符号を合わせる */
  double vecerr = 0.0;
  for (long k = 0; k < N; k++) {
    double d = sgn * x[k] - ve[k] / vn;
    vecerr += d * d;
  }
  vecerr = sqrt(vecerr);   /* x も ve/vn も単位ベクトルなので, これが相対 L2 誤差 */

  printf("n=%d, iters=%d, lambda_min=%.10f, analytic=%.10f, rel.err=%.2e\n",
         n, it, lambda_min, analytic, rel_err);
  printf("固有ベクトル(基本振動モード) 相対L2誤差=%.2e, 中央値=%.4f vs 隅の値=%.4f\n",
         vecerr, sgn * x[(n/2)*n + n/2], sgn * x[0]);
  printf("elapsed = %.3f sec\n", elapsed);
  free(x); free(y); free(Ax); free(ve);
  return 0;
}
