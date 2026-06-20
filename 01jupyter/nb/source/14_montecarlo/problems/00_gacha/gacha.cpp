#include <cstdio>
#include <cstdlib>
#include <cmath>

/* ── 状態を持たない (カウンタベースの) 乱数: たった1つの純粋関数 ──────────
   draw_rand(seed, k, N) は 0..N-1 の整数を返す。
   - seed は「どの乱数列 (ストリーム) を使うか」を選ぶ番号 (この問題では試行ごとに変える)。
   - k は「その列の何番目を取り出すか」。同じ seed でも k が違えば別の値。
   - 同じ (seed,k) なら必ず同じ値を返す純粋関数なので, どのスレッドが計算しても,
     引かれる乱数列はスレッド数によらず同一になる (共有状態が無いので競合もしない)。
   (教育用の簡単なハッシュ。M=2^31-1 未満で計算し, 途中の積も 64bit に収まる。)  */
static inline int draw_rand(long long seed, long long k, int N) {
  const long long M = 2147483647LL;   /* 2^31 - 1 */
  long long x = ((seed % M) * 2654435761LL + (k % M) + 1) % M;   /* seed と k を1つにまとめる */
  x = ((x ^ (x >> 16)) * 1812433253LL) % M;
  x = ((x ^ (x >> 13)) * 1664525LL)    % M;
  x =  (x ^ (x >> 16)) % M;
  return (int)(x % N);
}

/* 1試行: N 種類が等確率で出るとき, 全種類そろうまでに引いた回数。
   そろった種類を 64bit のビットマスクで管理する (N <= 62 を想定)。
   seed に試行番号を渡し, k=0,1,2,... と引いていく。 */
long one_trial(int N, long long seed) {
  unsigned long long got = 0;
  unsigned long long full = (N == 64 ? ~0ULL : ((1ULL << N) - 1));
  long k = 0;
  while (got != full) {
    got |= (1ULL << draw_rand(seed, k, N));
    k++;
  }
  return k;   /* 引いた回数 */
}

int main(int argc, char ** argv) {
  int  N = (argc > 1 ? atoi(argv[1]) : 10);          /* 景品の種類数 */
  long T = (argc > 2 ? atol(argv[2]) : 1000000);     /* 試行回数 */
  /* 引き回数は整数なので整数で集計する → 足す順番によらず答えが完全に一致する */
  long long total = 0, totalsq = 0;

  /* T 回の試行は互いに独立。各試行の引き回数を集計する。 */
  // BEGIN ANSWER: 各試行は独立なので #pragma omp parallel for reduction(+:total,totalsq) で並列化・集計せよ.
#pragma omp parallel for reduction(+:total,totalsq)
  // END ANSWER
  for (long t = 0; t < T; t++) {
    long d = one_trial(N, t);
    total   += d;
    totalsq += (long long)d * d;
  }

  double mean = (double)total / T;
  double var  = (double)totalsq / T - mean * mean;
  /* 理論期待値 = N * H_N (H_N は調和数) */
  double H = 0.0;
  for (int k = 1; k <= N; k++) H += 1.0 / k;
  printf("N=%d, trials=%ld: 平均 %.3f 回 (理論 %.3f), 標準偏差 %.3f\n",
         N, T, mean, N * H, sqrt(var));
  return 0;
}
