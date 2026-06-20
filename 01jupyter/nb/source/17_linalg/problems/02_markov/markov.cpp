#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <omp.h>

/* 状態を持たない乱数 (未使用だが慣例として置いておく): (seed,k) から [0,1)。 */
static inline double draw_rand01(long long seed, long long k) {
  const long long M = 2147483647LL;
  long long x = ((seed % M) * 2654435761LL + (k % M) + 1) % M;
  x = ((x ^ (x >> 16)) * 1812433253LL) % M;
  x = ((x ^ (x >> 13)) * 1664525LL)    % M;
  x =  (x ^ (x >> 16)) % M;
  return (double)x / (double)M;
}

/* ワープ (はしご/すべり台) の行き先を返す。from に該当しなければ d をそのまま返す。 */
static inline int warp(int d, int S) {
  if (d == 3)       return S / 2;
  if (d == S / 4)   return S - 2;
  if (d == S/2 + 5) return 1;
  if (d == S - 7)   return S / 3;
  return d;
}

int main(int argc, char ** argv) {
  int    S     = (argc > 1 ? atoi(argv[1]) : 1000);   /* マスの数 (0..S-1 の輪) */
  double tol   = (argc > 2 ? atof(argv[2]) : 1e-10);
  int    maxit = (argc > 3 ? atoi(argv[3]) : 100000);

  /* 遷移行列 M (密) を構築。M[t*S + s] = マス s から t へ1ターンで移る確率。
     各 s について サイコロ 1..6 を振り d=(s+roll)%S, ワープがあれば飛ばす。 */
  double * M = (double *)calloc((size_t)S * S, sizeof(double));
  for (int s = 0; s < S; s++)
    for (int roll = 1; roll <= 6; roll++) {
      int d = warp((s + roll) % S, S);
      M[(size_t)d * S + s] += 1.0 / 6.0;
    }

  double * pi  = (double *)malloc(sizeof(double) * S);
  double * pin = (double *)malloc(sizeof(double) * S);
  for (int s = 0; s < S; s++) pi[s] = 1.0 / S;        /* 一様分布から開始 */

  /* べき乗法: 遷移行列を繰り返し掛けると定常分布に収束する (最大固有値 = 1)。 */
  int it;
  double t0 = omp_get_wtime();
  for (it = 0; it < maxit; it++) {
    // BEGIN ANSWER: 行 t ごとの行列ベクトル積を #pragma omp parallel for で並列化せよ (各 t は独立).
#pragma omp parallel for
    // END ANSWER
    for (int t = 0; t < S; t++) {
      double sum = 0.0;
      for (int s = 0; s < S; s++) sum += M[(size_t)t * S + s] * pi[s];
      pin[t] = sum;
    }
    double total = 0.0;
    // BEGIN ANSWER: 総和を #pragma omp parallel for reduction(+:total) で並列化せよ.
#pragma omp parallel for reduction(+:total)
    // END ANSWER
    for (int t = 0; t < S; t++) total += pin[t];
    double diff = 0.0;
    for (int t = 0; t < S; t++) {
      pin[t] /= total;                                 /* 正規化 (sum=1) */
      double e = fabs(pin[t] - pi[t]); if (e > diff) diff = e;
      pi[t] = pin[t];
    }
    if (diff < tol) { it++; break; }
  }
  double elapsed = omp_get_wtime() - t0;

  /* 検算: sum(pi), 最も止まりやすいマスとその確率, 上位3マス */
  double sum = 0.0;
  for (int s = 0; s < S; s++) sum += pi[s];
  int best = 0;
  for (int s = 1; s < S; s++) if (pi[s] > pi[best]) best = s;

  /* 上位3マスを単純に探す */
  int top[3] = {-1, -1, -1};
  for (int r = 0; r < 3; r++) {
    int b = -1;
    for (int s = 0; s < S; s++) {
      bool used = false;
      for (int q = 0; q < r; q++) if (top[q] == s) used = true;
      if (used) continue;
      if (b < 0 || pi[s] > pi[b]) b = s;
    }
    top[r] = b;
  }

  printf("S=%d, iters=%d, sum=%.10f\n", S, it, sum);
  printf("最も止まりやすいマス=%d (確率 %.6f), 一様なら 1/S=%.6f\n",
         best, pi[best], 1.0 / S);
  printf("上位3マス: %d(%.6f), %d(%.6f), %d(%.6f)\n",
         top[0], pi[top[0]], top[1], pi[top[1]], top[2], pi[top[2]]);
  printf("elapsed = %.3f sec\n", elapsed);
  free(M); free(pi); free(pin);
  return 0;
}
