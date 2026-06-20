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

/* 群れの「整列度」(polarization): 全個体の進行方向の平均の大きさ。
   バラバラなら 0 に近く, みんな同じ向きなら 1 に近い。 */
double polarization(int N, const double * vx, const double * vy) {
  double sx = 0.0, sy = 0.0;
  for (int i = 0; i < N; i++) {
    double s = sqrt(vx[i]*vx[i] + vy[i]*vy[i]);
    sx += vx[i] / s; sy += vy[i] / s;
  }
  return sqrt(sx*sx + sy*sy) / N;
}

int main(int argc, char ** argv) {
  int N     = (argc > 1 ? atoi(argv[1]) : 1000);   /* 個体数 */
  int steps = (argc > 2 ? atoi(argv[2]) : 300);    /* 時間ステップ数 */
  /* ルールのパラメータ */
  double box = 30.0;        /* 正方形領域 (周期境界) */
  double R = 15.0, Rs = 2.0;/* 近傍半径, 分離半径 */
  double wc = 0.01, wa = 0.2, ws = 0.05;  /* 結合, 整列, 分離の強さ */
  double speed = 1.0, dt = 1.0;

  /* 現在(px..) と 次(qx..) の2組の配列 (Jacobi のように読みと書きを分ける) */
  double * px = (double *)malloc(sizeof(double) * N), * py = (double *)malloc(sizeof(double) * N);
  double * vx = (double *)malloc(sizeof(double) * N), * vy = (double *)malloc(sizeof(double) * N);
  double * qx = (double *)malloc(sizeof(double) * N), * qy = (double *)malloc(sizeof(double) * N);
  double * ux = (double *)malloc(sizeof(double) * N), * uy = (double *)malloc(sizeof(double) * N);
  for (int i = 0; i < N; i++) {
    px[i] = box * draw_rand01(i, 0);
    py[i] = box * draw_rand01(i, 1);
    double a = 2.0 * M_PI * draw_rand01(i, 2);   /* ランダムな初期方向 */
    vx[i] = cos(a); vy[i] = sin(a);
  }
  double P0 = polarization(N, vx, vy);

  double t0 = omp_get_wtime();
  for (int t = 0; t < steps; t++) {
    /* 各個体 i を, 近傍 j を見て更新する (近傍探索が O(N), 全体で O(N^2))。 */
    // TODO: 各個体 i のループを #pragma omp parallel for で並列化せよ (i ごとに独立)。
    for (int i = 0; i < N; i++) {
      double cx = 0, cy = 0, avx = 0, avy = 0, sx = 0, sy = 0;
      int cnt = 0;
      for (int j = 0; j < N; j++) {
        if (j == i) continue;
        double dx = px[j] - px[i], dy = py[j] - py[i];
        double d2 = dx*dx + dy*dy;
        if (d2 < R * R) {
          cx += px[j]; cy += py[j]; avx += vx[j]; avy += vy[j]; cnt++;
          if (d2 < Rs * Rs) { sx += px[i] - px[j]; sy += py[i] - py[j]; }  /* 分離: 近すぎる相手から離れる */
        }
      }
      double ax = 0, ay = 0;
      if (cnt > 0) {
        cx /= cnt; cy /= cnt; avx /= cnt; avy /= cnt;
        ax += wc * (cx - px[i]) + wa * (avx - vx[i]);   /* 結合 + 整列 */
        ay += wc * (cy - py[i]) + wa * (avy - vy[i]);
      }
      ax += ws * sx; ay += ws * sy;                     /* 分離 */
      double nvx = vx[i] + ax, nvy = vy[i] + ay;
      double s = sqrt(nvx*nvx + nvy*nvy); if (s < 1e-9) s = 1.0;
      nvx = nvx / s * speed; nvy = nvy / s * speed;     /* 速さは一定に保つ */
      ux[i] = nvx; uy[i] = nvy;
      qx[i] = fmod(px[i] + nvx * dt + box, box);        /* 周期境界 (はみ出たら反対側へ) */
      qy[i] = fmod(py[i] + nvy * dt + box, box);
    }
    /* 現在 <-> 次 を入れ替える */
    double * t1;
    t1 = px; px = qx; qx = t1;  t1 = py; py = qy; qy = t1;
    t1 = vx; vx = ux; ux = t1;  t1 = vy; vy = uy; uy = t1;
  }
  double elapsed = omp_get_wtime() - t0;

  printf("N=%d, steps=%d: 整列度 %.4f -> %.4f (1 に近いほど群れが揃った)\n",
         N, steps, P0, polarization(N, vx, vy));
  printf("elapsed = %.3f sec\n", elapsed);
  return 0;
}
