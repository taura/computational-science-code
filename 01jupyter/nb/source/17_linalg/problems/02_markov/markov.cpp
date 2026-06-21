#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <omp.h>

/* すごろくの定常分布をべき乗法で求める。遷移行列 M は密な S×S 行列 Mat,
   分布 pi は長さ S のベクトル Vec。M を繰り返し掛けて定常分布に収束させる。 */

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

struct Vec {
  long n;
  double * a;
  Vec(long n_)             : n(n_), a(new double[n_]) {}
  Vec(long n_, double * p) : n(n_), a(p) {}
  Vec(Vec && o) noexcept : n(o.n), a(o.a) { o.a = nullptr; }
  ~Vec() { delete[] a; }
  Vec(const Vec &) = delete;
  Vec & operator=(const Vec &) = delete;
  double & operator()(long i)       { return a[i]; }
  double   operator()(long i) const { return a[i]; }
  void zero() { for (long i = 0; i < n; i++) a[i] = 0.0; }
};

/* ワープ (はしご/すべり台) の行き先を返す。from に該当しなければ d をそのまま返す。 */
static inline int warp(int d, int S) {
  if (d == 3)       return S / 2;
  if (d == S / 4)   return S - 2;
  if (d == S/2 + 5) return 1;
  if (d == S - 7)   return S / 3;
  return d;
}

/* 行列ベクトル積 y = M x (密行列, 各行 t は独立) */
void matvec(const Mat & M, const Vec & x, Vec & y) {
  long S = M.rows;
  // TODO: 行 t ごとの行列ベクトル積を並列化する (各 t は独立)。
  // BEGIN ANSWER
#pragma omp parallel for
  // END ANSWER
  for (long t = 0; t < S; t++) {
    double sum = 0.0;
    for (long s = 0; s < S; s++) sum += M(t, s) * x(s);
    y(t) = sum;
  }
}

int main(int argc, char ** argv) {
  int    S     = (argc > 1 ? atoi(argv[1]) : 1000);   /* マスの数 (0..S-1 の輪) */
  double tol   = (argc > 2 ? atof(argv[2]) : 1e-10);
  int    maxit = (argc > 3 ? atoi(argv[3]) : 100000);

  /* 遷移行列 M (密) を構築。M(t,s) = マス s から t へ1ターンで移る確率。
     各 s について サイコロ 1..6 を振り d=(s+roll)%S, ワープがあれば飛ばす。 */
  Mat M(S, S);
  M.zero();
  for (int s = 0; s < S; s++)
    for (int roll = 1; roll <= 6; roll++) {
      int d = warp((s + roll) % S, S);
      M(d, s) += 1.0 / 6.0;
    }

  Vec pi(S), pin(S);
  for (int s = 0; s < S; s++) pi(s) = 1.0 / S;        /* 一様分布から開始 */

  /* べき乗法: 遷移行列を繰り返し掛けると定常分布に収束する (最大固有値 = 1)。 */
  int it;
  double t0 = omp_get_wtime();
  for (it = 0; it < maxit; it++) {
    matvec(M, pi, pin);
    double total = 0.0;
    // TODO: 総和を並列化する (reduction)。
    // BEGIN ANSWER
#pragma omp parallel for reduction(+:total)
    // END ANSWER
    for (long t = 0; t < S; t++) total += pin(t);
    double diff = 0.0;
    for (int t = 0; t < S; t++) {
      pin(t) /= total;                                 /* 正規化 (sum=1) */
      double e = fabs(pin(t) - pi(t)); if (e > diff) diff = e;
      pi(t) = pin(t);
    }
    if (diff < tol) { it++; break; }
  }
  double elapsed = omp_get_wtime() - t0;

  /* 検算: sum(pi), 最も止まりやすいマスとその確率, 上位3マス */
  double sum = 0.0;
  for (int s = 0; s < S; s++) sum += pi(s);
  int best = 0;
  for (int s = 1; s < S; s++) if (pi(s) > pi(best)) best = s;

  /* 上位3マスを単純に探す */
  int top[3] = {-1, -1, -1};
  for (int r = 0; r < 3; r++) {
    int b = -1;
    for (int s = 0; s < S; s++) {
      bool used = false;
      for (int q = 0; q < r; q++) if (top[q] == s) used = true;
      if (used) continue;
      if (b < 0 || pi(s) > pi(b)) b = s;
    }
    top[r] = b;
  }

  printf("S=%d, iters=%d, sum=%.10f\n", S, it, sum);
  printf("最も止まりやすいマス=%d (確率 %.6f), 一様なら 1/S=%.6f\n",
         best, pi(best), 1.0 / S);
  printf("上位3マス: %d(%.6f), %d(%.6f), %d(%.6f)\n",
         top[0], pi(top[0]), top[1], pi(top[1]), top[2], pi(top[2]));
  printf("elapsed = %.3f sec\n", elapsed);
  return 0;
}
