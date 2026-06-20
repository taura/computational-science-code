#include <cstdio>
#include <cstdlib>
#include <omp.h>

/* 状態を持たない (カウンタベースの) 乱数: (seed,k) から [0,1) の値を決める純粋関数。
   点 i の座標を draw_rand01(i,0), draw_rand01(i,1) で決めるので, どのスレッドが
   担当しても点 i の位置は同じ (共有状態が無いので競合しない)。
   (教育用の簡単なハッシュ。M=2^31-1 未満で計算し, 途中の積も 64bit に収まる。) */
static inline double draw_rand01(long long seed, long long k) {
  const long long M = 2147483647LL;   /* 2^31 - 1 */
  long long x = ((seed % M) * 2654435761LL + (k % M) + 1) % M;
  x = ((x ^ (x >> 16)) * 1812433253LL) % M;
  x = ((x ^ (x >> 13)) * 1664525LL)    % M;
  x =  (x ^ (x >> 16)) % M;
  return (double)x / (double)M;        /* [0,1) */
}

int main(int argc, char ** argv) {
  // 全体で投げる点の数 (コマンドライン引数, 既定 4,000,000)
  long N = (argc > 1) ? atol(argv[1]) : 4000000L;
  // BEGIN ANSWER: 下のブロックの直前に #pragma omp parallel を1行追加し, 各スレッドが自分の担当分の点を投げて, 自分の π 推定値を表示するようにせよ.
#pragma omp parallel
  // END ANSWER
  {
    int tid = omp_get_thread_num();
    int nt  = omp_get_num_threads();
    // このスレッドの担当する点の範囲 (全体 N 点を T スレッドで分割)
    long lo = (long)tid * N / nt;
    long hi = (long)(tid + 1) * N / nt;
    long my_n = hi - lo;
    long hits = 0;
    for (long i = lo; i < hi; i++) {
      double x = draw_rand01(i, 0);   // 点 i の x 座標
      double y = draw_rand01(i, 1);   // 点 i の y 座標
      if (x * x + y * y < 1.0) {
        hits++;
      }
    }
    // 単位正方形に対する 1/4 円の面積比 = π/4
    double pi = (my_n > 0) ? 4.0 * hits / my_n : 0.0;
    printf("thread %d of %d: %ld points, pi estimate = %f\n",
           tid, nt, my_n, pi);
  }
  return 0;
}
