#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <omp.h>

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
void compute_acc(int N, const double * pos, const double * mass, double * acc, double eps) {
  double eps2 = eps * eps;
  // BEGIN ANSWER: 各粒子 i のループを #pragma omp parallel for で並列化せよ (i ごとに独立)。
#pragma omp parallel for
  // END ANSWER
  for (int i = 0; i < N; i++) {
    double xi = pos[3*i], yi = pos[3*i+1], zi = pos[3*i+2];
    double ax = 0.0, ay = 0.0, az = 0.0;
    for (int j = 0; j < N; j++) {
      double dx = pos[3*j]   - xi;
      double dy = pos[3*j+1] - yi;
      double dz = pos[3*j+2] - zi;
      double r2 = dx*dx + dy*dy + dz*dz + eps2;
      double inv = 1.0 / (r2 * sqrt(r2));   /* 1/r^3 (ソフトニング込み) */
      double f = mass[j] * inv;
      ax += f * dx; ay += f * dy; az += f * dz;
    }
    acc[3*i] = ax; acc[3*i+1] = ay; acc[3*i+2] = az;
  }
}

/* 全エネルギー = 運動エネルギー + 位置エネルギー (検算用) */
double energy(int N, const double * pos, const double * vel, const double * mass, double eps) {
  double eps2 = eps * eps, KE = 0.0, PE = 0.0;
  for (int i = 0; i < N; i++) {
    KE += 0.5 * mass[i] * (vel[3*i]*vel[3*i] + vel[3*i+1]*vel[3*i+1] + vel[3*i+2]*vel[3*i+2]);
    for (int j = i + 1; j < N; j++) {
      double dx = pos[3*j]-pos[3*i], dy = pos[3*j+1]-pos[3*i+1], dz = pos[3*j+2]-pos[3*i+2];
      PE -= mass[i] * mass[j] / sqrt(dx*dx + dy*dy + dz*dz + eps2);
    }
  }
  return KE + PE;
}

int main(int argc, char ** argv) {
  int    N     = (argc > 1 ? atoi(argv[1]) : 2000);   /* 粒子数 */
  int    steps = (argc > 2 ? atoi(argv[2]) : 100);    /* 時間ステップ数 */
  double dt = 0.001, eps = 0.05;

  double * pos  = (double *)malloc(sizeof(double) * 3 * N);
  double * vel  = (double *)calloc(3 * N, sizeof(double));
  double * acc  = (double *)malloc(sizeof(double) * 3 * N);
  double * mass = (double *)malloc(sizeof(double) * N);
  /* 初期条件: [-1,1]^3 にランダムに配置, 速度 0, 質量は等しく合計 1。 */
  for (int i = 0; i < N; i++) {
    mass[i] = 1.0 / N;
    pos[3*i]   = 2.0 * draw_rand01(i, 0) - 1.0;
    pos[3*i+1] = 2.0 * draw_rand01(i, 1) - 1.0;
    pos[3*i+2] = 2.0 * draw_rand01(i, 2) - 1.0;
  }

  double E0 = energy(N, pos, vel, mass, eps);
  double t0 = omp_get_wtime();
  for (int t = 0; t < steps; t++) {
    compute_acc(N, pos, mass, acc, eps);
    /* シンプレクティック・オイラー法で時間を進める (v を更新してから x を更新) */
    for (int i = 0; i < 3 * N; i++) vel[i] += acc[i] * dt;
    for (int i = 0; i < 3 * N; i++) pos[i] += vel[i] * dt;
  }
  double elapsed = omp_get_wtime() - t0;
  double E1 = energy(N, pos, vel, mass, eps);

  printf("N=%d, steps=%d: エネルギー %.6e -> %.6e (相対変化 %.2e)\n",
         N, steps, E0, E1, fabs((E1 - E0) / E0));
  printf("elapsed = %.3f sec\n", elapsed);
  free(pos); free(vel); free(acc); free(mass);
  return 0;
}
