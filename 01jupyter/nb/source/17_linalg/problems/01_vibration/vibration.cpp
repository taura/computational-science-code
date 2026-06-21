#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <omp.h>

/* べき乗法で 2次元ラプラシアン (膜) の最小固有値=基本振動モードを求める。
   未知数は n×n 格子上に並ぶので, 各ベクトル (x,y,Ax,ve) を Mat(n,n) で表す。
   行列ベクトル積 A p は 5点ステンシルで計算する。 */

/* 連続メモリの row-major 行列: A(i,j) = a[i*cols + j] */
struct Mat {
  long rows, cols;
  double * a;
  Mat(long r, long c)             : rows(r), cols(c), a(new double[r * c]) {}
  Mat(long r, long c, double * p) : rows(r), cols(c), a(p) {}
  Mat(Mat && o) noexcept : rows(o.rows), cols(o.cols), a(o.a) { o.a = nullptr; }
  ~Mat() { delete[] a; }
  Mat(const Mat &) = delete;
  Mat & operator=(const Mat &) = delete;
  double & operator()(long i, long j)       { return a[i * cols + j]; }
  double   operator()(long i, long j) const { return a[i * cols + j]; }
  long size() const { return rows * cols; }
  void zero() { for (long i = 0; i < rows * cols; i++) a[i] = 0.0; }
};

/* 状態を持たない乱数 (初期ベクトルの生成用): (seed,k) から [0,1)。 */
static inline double draw_rand01(long long seed, long long k) {
  const long long M = 2147483647LL;
  long long x = ((seed % M) * 2654435761LL + (k % M) + 1) % M;
  x = ((x ^ (x >> 16)) * 1812433253LL) % M;
  x = ((x ^ (x >> 13)) * 1664525LL)    % M;
  x =  (x ^ (x >> 16)) % M;
  return (double)x / (double)M;
}

/* 行列ベクトル積 y = A p。A は n×n 格子上の 2次元ラプラシアン (5点ステンシル,
   ディリクレ境界=0)。(A p)(i,j) = 4 p(i,j) - 上下左右 (領域外は 0)。 */
void matvec(const Mat & p, Mat & y) {
  long n = p.rows;
  // TODO: 格子点の二重ループを並列化する。
  // BEGIN ANSWER
#pragma omp parallel for collapse(2)
  // END ANSWER
  for (long i = 0; i < n; i++) {
    for (long j = 0; j < n; j++) {
      double v = 4.0 * p(i, j);
      if (i > 0)     v -= p(i - 1, j);
      if (i < n - 1) v -= p(i + 1, j);
      if (j > 0)     v -= p(i, j - 1);
      if (j < n - 1) v -= p(i, j + 1);
      y(i, j) = v;
    }
  }
}

/* 内積 a・b (ベクトルとみなして全要素の積和) */
double dot(const Mat & a, const Mat & b) {
  long N = a.size();
  double s = 0.0;
  // TODO: 内積の和を並列化する (reduction)。
  // BEGIN ANSWER
#pragma omp parallel for reduction(+:s)
  // END ANSWER
  for (long k = 0; k < N; k++) s += a.a[k] * b.a[k];
  return s;
}

int main(int argc, char ** argv) {
  int    n     = (argc > 1 ? atoi(argv[1]) : 128);   /* 格子の一辺 (未知数は N = n*n) */
  double tol   = (argc > 2 ? atof(argv[2]) : 1e-9);
  int    maxit = (argc > 3 ? atoi(argv[3]) : 100000);
  long   N     = (long)n * n;
  double sigma = 8.0;                                /* シフト量 (> lambda_max ≈ 8) */

  Mat x(n, n), y(n, n), Ax(n, n);

  /* 初期ベクトル: すべて 1 から始めて正規化 */
  for (long k = 0; k < N; k++) x.a[k] = 1.0;
  double nrm0 = sqrt(dot(x, x));
  for (long k = 0; k < N; k++) x.a[k] /= nrm0;

  /* べき乗法: B = sigma*I - A を繰り返し掛ける。
     B の最大固有値 = sigma - lambda_min(A) に収束し, 固有ベクトルは基本振動モード。 */
  int it;
  double lamB = 0.0, lamB_prev = 0.0;
  double t0 = omp_get_wtime();
  for (it = 0; it < maxit; it++) {
    matvec(x, Ax);
    for (long k = 0; k < N; k++) y.a[k] = sigma * x.a[k] - Ax.a[k];   /* y = B x (逐次のまま) */
    lamB = dot(x, y) / dot(x, x);                                     /* レイリー商 */
    double nrm = sqrt(dot(y, y));
    for (long k = 0; k < N; k++) x.a[k] = y.a[k] / nrm;               /* 正規化 (逐次のまま) */
    if (it > 0 && fabs(lamB - lamB_prev) < tol) { it++; break; }
    lamB_prev = lamB;
  }
  double elapsed = omp_get_wtime() - t0;

  double lambda_min = sigma - lamB;
  double analytic   = 4.0 - 4.0 * cos(M_PI / (n + 1));
  double rel_err    = fabs(lambda_min - analytic) / analytic;

  /* 固有ベクトルの検算: 解析解 v(i,j) = sin(pi(i+1)/(n+1)) sin(pi(j+1)/(n+1)) と比べる。
     まず符号を合わせ, 正規化した両者の相対 L2 誤差を計算する。 */
  Mat ve(n, n);
  double vn = 0.0;
  for (long i = 0; i < n; i++)
    for (long j = 0; j < n; j++) {
      double v = sin(M_PI * (i+1) / (n+1)) * sin(M_PI * (j+1) / (n+1));
      ve(i, j) = v; vn += v * v;
    }
  vn = sqrt(vn);
  double sgn = (x(n/2, n/2) >= 0.0) ? 1.0 : -1.0;   /* 中央の値で符号を合わせる */
  double vecerr = 0.0;
  for (long k = 0; k < N; k++) {
    double d = sgn * x.a[k] - ve.a[k] / vn;
    vecerr += d * d;
  }
  vecerr = sqrt(vecerr);   /* x も ve/vn も単位ベクトルなので, これが相対 L2 誤差 */

  printf("n=%d, iters=%d, lambda_min=%.10f, analytic=%.10f, rel.err=%.2e\n",
         n, it, lambda_min, analytic, rel_err);
  printf("固有ベクトル(基本振動モード) 相対L2誤差=%.2e, 中央値=%.4f vs 隅の値=%.4f\n",
         vecerr, sgn * x(n/2, n/2), sgn * x(0, 0));
  printf("elapsed = %.3f sec\n", elapsed);
  return 0;
}
