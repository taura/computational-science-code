#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <omp.h>

/* 2D 定常熱伝導 (ラプラス方程式) をヤコビ反復で解く。
   L×L の板。上端(行0)の温度を 100, 残り3辺を 0 に固定し, 内部が定常分布に
   落ち着くまで反復する。各内部点を上下左右4点の平均で更新する (5点ステンシル)。
   2次元配列は Mat で表す (u(i,j) でアクセス, 中身は連続メモリ)。 */

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
  void zero() { for (long i = 0; i < rows * cols; i++) a[i] = 0.0; }
};

int main(int argc, char ** argv) {
  int    L       = (argc > 1 ? atoi(argv[1]) : 129);     /* 一辺の格子点数 */
  double tol     = (argc > 2 ? atof(argv[2]) : 1e-6);    /* 収束判定 (最大更新量) */
  int    maxiter = (argc > 3 ? atoi(argv[3]) : 1000000);

  Mat u(L, L), unew(L, L);
  /* 初期化: 上端(行0)=100, 他=0。境界はずっと固定なので両配列に同じ値を入れておく。 */
  for (int i = 0; i < L; i++)
    for (int j = 0; j < L; j++) {
      double val = (i == 0) ? 100.0 : 0.0;
      u(i, j) = val;
      unew(i, j) = val;
    }

  int iter;
  double diff = 0.0;
  double t0 = omp_get_wtime();
  for (iter = 0; iter < maxiter; iter++) {
    diff = 0.0;   /* この反復での最大更新量 */
    /* 内部の各点 (i,j) を上下左右の平均で更新し, 更新量の最大値を求める。 */
    // TODO: この内側の二重ループ (各内部点の更新) を並列化する。
    // BEGIN ANSWER
#pragma omp parallel for collapse(2) reduction(max:diff)
    // END ANSWER
    for (int i = 1; i < L - 1; i++) {
      for (int j = 1; j < L - 1; j++) {
        double v = 0.25 * (u(i - 1, j) + u(i + 1, j) + u(i, j - 1) + u(i, j + 1));
        double d = fabs(v - u(i, j));
        if (d > diff) diff = d;
        unew(i, j) = v;
      }
    }
    /* u と unew を入れ替える (コピーせず中身のポインタを差し替えるだけ) */
    double * tmp = u.a; u.a = unew.a; unew.a = tmp;
    if (diff < tol) break;
  }
  double elapsed = omp_get_wtime() - t0;

  printf("L=%d, iters=%d, 最終残差=%.2e, 中心温度=%.4f (理論 25.0)\n",
         L, iter + 1, diff, u(L / 2, L / 2));
  printf("elapsed = %.3f sec\n", elapsed);
  return 0;
}
