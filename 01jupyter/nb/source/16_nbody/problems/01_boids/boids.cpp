#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <omp.h>

/* Boids (群れ) シミュレーション。各個体の位置・速度は N×2 の行列 Mat で表す
   (pos(i,0),pos(i,1) が個体 i の x,y)。現在 (pos,vel) と次 (qpos,qvel) の
   2組を使い, Jacobi のように読みと書きを分ける。 */

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
double polarization(const Mat & vel) {
  long N = vel.rows;
  double sx = 0.0, sy = 0.0;
  for (long i = 0; i < N; i++) {
    double s = sqrt(vel(i,0)*vel(i,0) + vel(i,1)*vel(i,1));
    sx += vel(i,0) / s; sy += vel(i,1) / s;
  }
  return sqrt(sx*sx + sy*sy) / N;
}

int main(int argc, char ** argv) {
  int N     = (argc > 1 ? atoi(argv[1]) : 1500);   /* 個体数 */
  int steps = (argc > 2 ? atoi(argv[2]) : 300);    /* 時間ステップ数 */
  /* ルールのパラメータ */
  double box = 30.0;        /* 正方形領域 (周期境界) */
  double R = 15.0, Rs = 2.0;/* 近傍半径, 分離半径 */
  double wc = 0.01, wa = 0.2, ws = 0.05;  /* 結合, 整列, 分離の強さ */
  double speed = 1.0, dt = 1.0;

  /* 現在 (pos,vel) と 次 (qpos,qvel) の2組 (読みと書きを分ける) */
  Mat pos(N, 2), vel(N, 2), qpos(N, 2), qvel(N, 2);
  for (int i = 0; i < N; i++) {
    pos(i, 0) = box * draw_rand01(i, 0);
    pos(i, 1) = box * draw_rand01(i, 1);
    double a = 2.0 * M_PI * draw_rand01(i, 2);   /* ランダムな初期方向 */
    vel(i, 0) = cos(a); vel(i, 1) = sin(a);
  }
  double P0 = polarization(vel);

  double t0 = omp_get_wtime();
  for (int t = 0; t < steps; t++) {
    /* 各個体 i を, 近傍 j を見て更新する (近傍探索が O(N), 全体で O(N^2))。 */
    // TODO: 各個体 i のループを並列化する (i ごとに独立)。
    // BEGIN ANSWER
#pragma omp parallel for
    // END ANSWER
    for (long i = 0; i < N; i++) {
      double xi = pos(i,0), yi = pos(i,1);
      double cx = 0, cy = 0, avx = 0, avy = 0, sx = 0, sy = 0;
      int cnt = 0;
      for (long j = 0; j < N; j++) {
        if (j == i) continue;
        double dx = pos(j,0) - xi, dy = pos(j,1) - yi;
        double d2 = dx*dx + dy*dy;
        if (d2 < R * R) {
          cx += pos(j,0); cy += pos(j,1); avx += vel(j,0); avy += vel(j,1); cnt++;
          if (d2 < Rs * Rs) { sx += xi - pos(j,0); sy += yi - pos(j,1); }  /* 分離: 近すぎる相手から離れる */
        }
      }
      double ax = 0, ay = 0;
      if (cnt > 0) {
        cx /= cnt; cy /= cnt; avx /= cnt; avy /= cnt;
        ax += wc * (cx - xi) + wa * (avx - vel(i,0));   /* 結合 + 整列 */
        ay += wc * (cy - yi) + wa * (avy - vel(i,1));
      }
      ax += ws * sx; ay += ws * sy;                     /* 分離 */
      double nvx = vel(i,0) + ax, nvy = vel(i,1) + ay;
      double s = sqrt(nvx*nvx + nvy*nvy); if (s < 1e-9) s = 1.0;
      nvx = nvx / s * speed; nvy = nvy / s * speed;     /* 速さは一定に保つ */
      qvel(i,0) = nvx; qvel(i,1) = nvy;
      qpos(i,0) = fmod(xi + nvx * dt + box, box);       /* 周期境界 (はみ出たら反対側へ) */
      qpos(i,1) = fmod(yi + nvy * dt + box, box);
    }
    /* 現在 <-> 次 を入れ替える (中身のポインタを差し替えるだけ) */
    double * t1;
    t1 = pos.a; pos.a = qpos.a; qpos.a = t1;
    t1 = vel.a; vel.a = qvel.a; qvel.a = t1;
  }
  double elapsed = omp_get_wtime() - t0;

  printf("N=%d, steps=%d: 整列度 %.4f -> %.4f (1 に近いほど群れが揃った)\n",
         N, steps, P0, polarization(vel));
  printf("elapsed = %.3f sec\n", elapsed);
  return 0;
}
