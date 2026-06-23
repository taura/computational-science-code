#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <omp.h>

/* 物理情報ニューラルネット (PINN) で微分方程式
     -u''(x) = f(x),  u(0)=u(1)=0
   を「ニューラルネット」で解く。差分法(B系)・CG(G1)と同じ問題を機械学習で解く。

   1隠れ層・tanh のネットなので u の微分は解析的に書ける (autodiff 不要):
     u(x)   = Σ_k a_k tanh(z_k),         z_k = w_k x + b_k
     u'(x)  = Σ_k a_k w_k   sech^2(z_k),  sech^2 = 1 - tanh^2
     u''(x) = Σ_k a_k w_k^2 (-2 tanh(z_k) sech^2(z_k))

   厳密解 u*(x) = sin(pi x) (u(0)=u(1)=0 を満たす) を仕掛けると f(x) = pi^2 sin(pi x)。
   損失 = (1/M) Σ_i ( -u''(x_i) - f(x_i) )^2 + lambda_bc ( u(0)^2 + u(1)^2 )。
   パラメータ {a_k, w_k, b_k} (3H 個) を勾配降下で学習する。
   勾配はパラメータについての差分 (有限差分) で求める:
     grad_j = ( loss(p + eps e_j) - loss(p - eps e_j) ) / (2 eps)。
   この 3H 個の評価は互いに独立なので, パラメータ番号 j で並列化できる。
   パラメータ p や格子点 xs は長さが決まったベクトル Vec で表す。 */

/* 連続メモリのベクトル: v(i) = a[i] */
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

static const double PI = 3.14159265358979323846;

/* 状態を持たない乱数 (パラメータ初期化用): (seed,k) から [0,1)。 */
static inline double draw_rand01(long long seed, long long k) {
  const long long M = 2147483647LL;
  long long x = ((seed % M) * 2654435761LL + (k % M) + 1) % M;
  x = ((x ^ (x >> 16)) * 1812433253LL) % M;
  x = ((x ^ (x >> 13)) * 1664525LL)    % M;
  x =  (x ^ (x >> 16)) % M;
  return (double)x / (double)M;
}

/* 損失をパラメータ配列 p (長さ 3H: a[0..H-1], w[H..2H-1], b[2H..3H-1]) の純関数として
   計算する (スレッドセーフ)。xs は格子点 (長さ M)。 */
static double loss_fn(int H, const double * p, int M, const double * xs, double lambda_bc) {
  const double * a = p;
  const double * w = p + H;
  const double * b = p + 2*H;
  /* PDE 残差項 */
  double res = 0.0;
  for (int i = 0; i < M; i++) {
    double x = xs[i];
    double upp = 0.0;        /* u''(x) */
    for (int k = 0; k < H; k++) {
      double z = w[k]*x + b[k];
      double t = tanh(z);
      double s2 = 1.0 - t*t;            /* sech^2 */
      upp += a[k] * w[k]*w[k] * (-2.0*t*s2);
    }
    double f = PI*PI*sin(PI*x);         /* f = -u*'' = pi^2 sin(pi x) */
    double r = -upp - f;
    res += r*r;
  }
  res /= (double)M;
  /* 境界項: u(0), u(1) */
  double u0 = 0.0, u1 = 0.0;
  for (int k = 0; k < H; k++) {
    u0 += a[k] * tanh(b[k]);
    u1 += a[k] * tanh(w[k] + b[k]);
  }
  return res + lambda_bc * (u0*u0 + u1*u1);
}

int main(int argc, char ** argv) {
  int    H     = (argc > 1 ? atoi(argv[1]) : 16);     /* 隠れユニット数 */
  int    M     = (argc > 2 ? atoi(argv[2]) : 60);     /* コロケーション点数 */
  int    steps = (argc > 3 ? atoi(argv[3]) : 4000);   /* 勾配降下ステップ数 */
  double lr    = (argc > 4 ? atof(argv[4]) : 0.01);   /* 学習率 */
  double lambda_bc = 10.0;
  double eps = 1e-5;
  int P = 3*H;                                        /* パラメータ総数 */

  /* コロケーション点 x_i = (i+0.5)/M を (0,1) に等間隔配置 */
  Vec xs(M);
  for (int i = 0; i < M; i++) xs(i) = (i + 0.5) / M;

  /* パラメータ p[3H] = {a, w, b} を小さな乱数で初期化 */
  Vec p(P), grad(P);
  for (int k = 0; k < H; k++) {
    p(k)       = (draw_rand01(k, 1) - 0.5) * 0.2;   /* a */
    p(H+k)     = (draw_rand01(k, 2) - 0.5) * 4.0;   /* w */
    p(2*H+k)   = (draw_rand01(k, 3) - 0.5) * 2.0;   /* b */
  }

  double t0 = omp_get_wtime();
  for (int it = 0; it < steps; it++) {
    /* パラメータについての差分勾配。各 j は独立 (loss は純関数なので各スレッドが
       自分のコピーを摂動して評価する)。 */
    // TODO: 3H 個のパラメータの差分勾配ループを並列化する (各 j は独立, loss を2回評価)。
    // BEGIN ANSWER
#pragma omp parallel for
    // END ANSWER
    for (int j = 0; j < P; j++) {
      Vec pp(P);
      for (int q = 0; q < P; q++) pp.a[q] = p.a[q];
      pp.a[j] = p.a[j] + eps; double lp = loss_fn(H, pp.a, M, xs.a, lambda_bc);
      pp.a[j] = p.a[j] - eps; double lm = loss_fn(H, pp.a, M, xs.a, lambda_bc);
      grad.a[j] = (lp - lm) / (2.0 * eps);
    }
    for (int j = 0; j < P; j++) p.a[j] -= lr * grad.a[j];

    if (it % 1000 == 0 || it == steps - 1) {
      double L = loss_fn(H, p.a, M, xs.a, lambda_bc);
      printf("step %4d: loss=%.6e\n", it, L);
    }
  }
  double elapsed = omp_get_wtime() - t0;

  /* 検算: 学習した u(x) を厳密解 sin(pi x) と比べる */
  const double * a = p.a; const double * w = p.a + H; const double * b = p.a + 2*H;
  double maxerr = 0.0;
  int NC = 101;
  for (int i = 0; i < NC; i++) {
    double x = (double)i / (NC - 1);
    double u = 0.0;
    for (int k = 0; k < H; k++) u += a[k] * tanh(w[k]*x + b[k]);
    double e = fabs(u - sin(PI*x));
    if (e > maxerr) maxerr = e;
  }
  printf("H=%d, M=%d, steps=%d, lr=%g\n", H, M, steps, lr);
  printf("final max|u - sin(pi x)| = %.4e\n", maxerr);
  printf("elapsed = %.3f sec\n", elapsed);
  return 0;
}
