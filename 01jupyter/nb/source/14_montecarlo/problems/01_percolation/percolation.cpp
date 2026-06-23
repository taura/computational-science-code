#include <cstdio>
#include <cstdlib>
#include <omp.h>

/* 状態を持たない (カウンタベースの) 乱数: (seed,k) から [0,1) の値を決める純粋関数。
   セル k の開閉を draw_rand01(seed=試行番号, k=セル番号) で決めるので, どのスレッドが
   担当しても同じ格子になり, スレッド数によらず結果が一致する (共有状態なし=競合なし)。 */
static inline double draw_rand01(long long seed, long long k) {
  const long long M = 2147483647LL;   /* 2^31 - 1 */
  long long x = ((seed % M) * 2654435761LL + (k % M) + 1) % M;
  x = ((x ^ (x >> 16)) * 1812433253LL) % M;
  x = ((x ^ (x >> 13)) * 1664525LL)    % M;
  x =  (x ^ (x >> 16)) % M;
  return (double)x / (double)M;        /* [0,1) */
}

/* 1試行: L×L の各セルを確率 p で「開」にした格子で,
   上端の開セルから下端の開セルへ (上下左右の開セル伝いに) たどり着けるか。
   たどり着ければ 1 (浸透した), さもなくば 0 を返す。深さ優先探索 (スタック) で判定。 */
int one_trial(int L, double p, long long seed) {
  int n = L * L;
  unsigned char * open = new unsigned char[n];
  unsigned char * vis  = new unsigned char[n]();    /* () で 0 初期化 */
  int *           stk  = new int[n];
  for (int i = 0; i < n; i++) open[i] = (draw_rand01(seed, i) < p) ? 1 : 0;

  int sp = 0;
  for (int c = 0; c < L; c++) {               /* 上端 (行0) の開セルを出発点に */
    if (open[c]) { vis[c] = 1; stk[sp++] = c; }
  }
  int perc = 0;
  const int dr[4] = { -1, 1, 0, 0 };
  const int dc[4] = { 0, 0, -1, 1 };
  while (sp > 0) {
    int idx = stk[--sp];
    int r = idx / L, c = idx % L;
    if (r == L - 1) { perc = 1; break; }      /* 下端に到達 = 浸透 */
    for (int d = 0; d < 4; d++) {
      int nr = r + dr[d], nc = c + dc[d];
      if (nr < 0 || nr >= L || nc < 0 || nc >= L) continue;
      int nidx = nr * L + nc;
      if (open[nidx] && !vis[nidx]) { vis[nidx] = 1; stk[sp++] = nidx; }
    }
  }
  delete[] open; delete[] vis; delete[] stk;
  return perc;
}

int main(int argc, char ** argv) {
  int    L = (argc > 1 ? atoi(argv[1]) : 128);     /* 格子の一辺 */
  double p = (argc > 2 ? atof(argv[2]) : 0.6);     /* セルが開く確率 */
  long   T = (argc > 3 ? atol(argv[3]) : 15000);    /* 試行回数 */
  long   perc = 0;

  /* T 回の試行は互いに独立。浸透した回数を数える。
     試行ごとに探索量が違う (浸透すると途中で打ち切る) ので schedule(dynamic) が有効。 */
  double t0 = omp_get_wtime();
  // TODO: この試行ループを並列化する (各試行は独立)。
  // BEGIN ANSWER
#pragma omp parallel for reduction(+:perc) schedule(dynamic)
  // END ANSWER
  for (long t = 0; t < T; t++) {
    perc += one_trial(L, p, t);
  }
  double elapsed = omp_get_wtime() - t0;
  printf("L=%d, p=%.3f, trials=%ld: 浸透確率 = %.4f\n", L, p, T, (double)perc / T);
  printf("elapsed = %.3f sec\n", elapsed);
  return 0;
}
