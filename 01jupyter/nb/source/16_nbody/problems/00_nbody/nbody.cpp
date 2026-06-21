#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <omp.h>

/* 直接法 (O(N^2)) の重力 N 体シミュレーション。
   位置・速度・加速度は N×3 の行列 Mat (pos(i,0..2) が粒子 i の x,y,z),
   質量は長さ N のベクトル Vec で表す。 */

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

/* 状態を持たない乱数 (初期配置の再現性のため): (seed,k) から [0,1)。 */
static inline double draw_rand01(long long seed, long long k) {
  const long long M = 2147483647LL;
  long long x = ((seed % M) * 2654435761LL + (k % M) + 1) % M;
  x = ((x ^ (x >> 16)) * 1812433253LL) % M;
  x = ((x ^ (x >> 13)) * 1664525LL)    % M;
  x =  (x ^ (x >> 16)) % M;
  return (double)x / (double)M;
}

/* 各粒子 i に働く加速度 = 他の全粒子 j からの重力の和 (直接法, O(N^2))。
   ソフトニング eps で近距離の発散を防ぐ。G=1。
   (j=i の項は dx=0 なので寄与 0。特別扱い不要。) */
void compute_acc(const Mat & pos, const Vec & mass, Mat & acc, double eps) {
  long N = pos.rows;
  double eps2 = eps * eps;
  // TODO: 各粒子 i のループを並列化する (i ごとに独立)。
  // BEGIN ANSWER
#pragma omp parallel for
  // END ANSWER
  for (long i = 0; i < N; i++) {
    double xi = pos(i, 0), yi = pos(i, 1), zi = pos(i, 2);
    double ax = 0.0, ay = 0.0, az = 0.0;
    for (long j = 0; j < N; j++) {
      double dx = pos(j, 0) - xi;
      double dy = pos(j, 1) - yi;
      double dz = pos(j, 2) - zi;
      double r2 = dx*dx + dy*dy + dz*dz + eps2;
      double inv = 1.0 / (r2 * sqrt(r2));   /* 1/r^3 (ソフトニング込み) */
      double f = mass(j) * inv;
      ax += f * dx; ay += f * dy; az += f * dz;
    }
    acc(i, 0) = ax; acc(i, 1) = ay; acc(i, 2) = az;
  }
}

/* 全エネルギー = 運動エネルギー + 位置エネルギー (検算用) */
double energy(const Mat & pos, const Mat & vel, const Vec & mass, double eps) {
  long N = pos.rows;
  double eps2 = eps * eps, KE = 0.0, PE = 0.0;
  for (long i = 0; i < N; i++) {
    KE += 0.5 * mass(i) * (vel(i,0)*vel(i,0) + vel(i,1)*vel(i,1) + vel(i,2)*vel(i,2));
    for (long j = i + 1; j < N; j++) {
      double dx = pos(j,0)-pos(i,0), dy = pos(j,1)-pos(i,1), dz = pos(j,2)-pos(i,2);
      PE -= mass(i) * mass(j) / sqrt(dx*dx + dy*dy + dz*dz + eps2);
    }
  }
  return KE + PE;
}

int main(int argc, char ** argv) {
  int    N     = (argc > 1 ? atoi(argv[1]) : 2000);   /* 粒子数 */
  int    steps = (argc > 2 ? atoi(argv[2]) : 100);    /* 時間ステップ数 */
  double dt = 0.001, eps = 0.05;

  Mat pos(N, 3), vel(N, 3), acc(N, 3);
  Vec mass(N);
  vel.zero();
  /* 初期条件: [-1,1]^3 にランダムに配置, 速度 0, 質量は等しく合計 1。 */
  for (int i = 0; i < N; i++) {
    mass(i) = 1.0 / N;
    pos(i, 0) = 2.0 * draw_rand01(i, 0) - 1.0;
    pos(i, 1) = 2.0 * draw_rand01(i, 1) - 1.0;
    pos(i, 2) = 2.0 * draw_rand01(i, 2) - 1.0;
  }

  double E0 = energy(pos, vel, mass, eps);
  double t0 = omp_get_wtime();
  for (int t = 0; t < steps; t++) {
    compute_acc(pos, mass, acc, eps);
    /* シンプレクティック・オイラー法で時間を進める (v を更新してから x を更新) */
    for (long k = 0; k < 3L * N; k++) vel.a[k] += acc.a[k] * dt;
    for (long k = 0; k < 3L * N; k++) pos.a[k] += vel.a[k] * dt;
  }
  double elapsed = omp_get_wtime() - t0;
  double E1 = energy(pos, vel, mass, eps);

  printf("N=%d, steps=%d: エネルギー %.6e -> %.6e (相対変化 %.2e)\n",
         N, steps, E0, E1, fabs((E1 - E0) / E0));
  printf("elapsed = %.3f sec\n", elapsed);
  return 0;
}
