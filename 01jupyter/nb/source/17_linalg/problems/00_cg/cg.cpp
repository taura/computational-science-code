#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <omp.h>

/* 共役勾配法 (CG) で 2次元ラプラシアン方程式 A x = b を解く (行列フリー)。
   未知数は n×n 格子上に並ぶので, CG の各ベクトル (x,b,r,p,Ap,xt) を Mat(n,n) で
   表す。行列ベクトル積 A p は 5点ステンシルで計算する (A は対称正定値)。
   内積や軸演算はベクトルとして中身 (.a) を一列に走査する。 */

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

/* 状態を持たない乱数 (既知解の生成用): (seed,k) から [0,1)。 */
static inline double draw_rand01(long long seed, long long k) {
  const long long M = 2147483647LL;
  long long x = ((seed % M) * 2654435761LL + (k % M) + 1) % M;
  x = ((x ^ (x >> 16)) * 1812433253LL) % M;
  x = ((x ^ (x >> 13)) * 1664525LL)    % M;
  x =  (x ^ (x >> 16)) % M;
  return (double)x / (double)M;
}

/* 行列ベクトル積 y = A p。A は n×n 格子上の 2次元ラプラシアン (5点ステンシル,
   ディリクレ境界=0) で対称正定値。行列を保持せずステンシルの計算で済ませる。
   (A p)(i,j) = 4 p(i,j) - p(i-1,j) - p(i+1,j) - p(i,j-1) - p(i,j+1)  (領域外は 0)。 */
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
  int    n   = (argc > 1 ? atoi(argv[1]) : 128);    /* 格子の一辺 (未知数は N = n*n) */
  double tol = (argc > 2 ? atof(argv[2]) : 1e-8);
  long   N   = (long)n * n;
  int    maxit = 10 * n;

  Mat xt(n, n), b(n, n), x(n, n), r(n, n), p(n, n), Ap(n, n);

  for (long k = 0; k < N; k++) xt.a[k] = draw_rand01(k, 0);  /* 真の解をランダムに決め */
  matvec(xt, b);                                             /* b = A xt を作る */

  /* CG: x=0 から始めて A x = b を解く */
  for (long k = 0; k < N; k++) { x.a[k] = 0.0; r.a[k] = b.a[k]; p.a[k] = b.a[k]; }
  double rs = dot(r, r);

  int it;
  double t0 = omp_get_wtime();
  for (it = 0; it < maxit; it++) {
    matvec(p, Ap);
    double alpha = rs / dot(p, Ap);
    for (long k = 0; k < N; k++) { x.a[k] += alpha * p.a[k]; r.a[k] -= alpha * Ap.a[k]; }  /* (発展: ここも並列化可) */
    double rs_new = dot(r, r);
    if (sqrt(rs_new) < tol) { rs = rs_new; it++; break; }
    double beta = rs_new / rs;
    for (long k = 0; k < N; k++) p.a[k] = r.a[k] + beta * p.a[k];
    rs = rs_new;
  }
  double elapsed = omp_get_wtime() - t0;

  /* 検算: 求めた x が真の解 xt にどれだけ近いか */
  double err = 0.0;
  for (long k = 0; k < N; k++) { double e = fabs(x.a[k] - xt.a[k]); if (e > err) err = e; }
  printf("n=%d (N=%ld), iters=%d, 残差=%.2e, 解の誤差(max|x-xt|)=%.2e\n",
         n, N, it, sqrt(rs), err);
  printf("elapsed = %.3f sec\n", elapsed);
  return 0;
}
