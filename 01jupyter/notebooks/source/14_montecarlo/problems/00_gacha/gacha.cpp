#include <cstdio>
#include <cstdlib>
#include <cmath>

/* ── 状態を持たない (カウンタベースの) 乱数 ──────────────────────────
   mix(x) は「整数 x → よく混ざった整数」を返す純粋関数 (状態を持たない)。
   draw(t,k,N) は「t 回目の試行の k 回目の引き」に対応する 0..N-1 の値を返す。
   同じ (t,k) なら必ず同じ値 → スレッドで分担しても引かれる乱数列は変わらない。
   (教育用の簡単なハッシュ。統計的品質は本格的な乱数生成器には及ばない。)
   M=2^31-1 未満で計算し, 途中の積も 64bit に収まるので桁あふれしない。      */
static const long long M = 2147483647LL;   /* 2^31 - 1 */

static inline long long mix(long long x) {
  x = ((x ^ (x >> 16)) * 1812433253LL) % M;
  x = ((x ^ (x >> 13)) * 1664525LL)    % M;
  x =  (x ^ (x >> 16)) % M;
  return x;
}

/* t 回目の試行の k 回目の引きで出る景品番号 (0..N-1) */
static inline int draw(long long t, long long k, int N) {
  long long key = ((t + 1) * 2654435761LL + (k + 1)) % M;
  return (int)(mix(key) % N);
}

/* 1試行: N 種類が等確率で出るとき, 全種類そろうまでに引いた回数。
   そろった種類を 64bit のビットマスクで管理する (N <= 62 を想定)。 */
long one_trial(int N, long long t) {
  unsigned long long got = 0;
  unsigned long long full = (N == 64 ? ~0ULL : ((1ULL << N) - 1));
  long k = 0;
  while (got != full) {
    got |= (1ULL << draw(t, k, N));
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
  // TODO: 各試行は独立なので #pragma omp parallel for reduction(+:total,totalsq) で並列化・集計せよ.
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
