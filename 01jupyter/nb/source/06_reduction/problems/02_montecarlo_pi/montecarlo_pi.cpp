#include <cstdio>
#include <cstdlib>

/* 状態を持たない (カウンタベースの) 乱数: (seed,k) から [0,1) の値を決める純粋関数。
   - seed は「どの乱数列(ストリーム)か」, k は「その何番目か」。同じ (seed,k) なら必ず同じ値。
   - 毎回 (seed,k) から計算し共有状態を持たないので, 並列化しても引かれる乱数列は
     スレッド数によらず同一になる (競合も起きない)。
   (教育用の簡単なハッシュ。M=2^31-1 未満で計算し, 途中の積も 64bit に収まる。)        */
static inline double draw_rand01(long long seed, long long k) {
  const long long M = 2147483647LL;   /* 2^31 - 1 */
  long long x = ((seed % M) * 2654435761LL + (k % M) + 1) % M;
  x = ((x ^ (x >> 16)) * 1812433253LL) % M;
  x = ((x ^ (x >> 13)) * 1664525LL)    % M;
  x =  (x ^ (x >> 16)) % M;
  return (double)x / (double)M;        /* [0,1) */
}

int main(int argc, char ** argv) {
  long n = (1 < argc ? atol(argv[1]) : 100L * 1000L * 1000L);
  long count = 0;               /* 単位円の 1/4 の内側に入った点数 */
  printf("n = %ld\n", n);
  /* 単位正方形 [0,1)x[0,1) に n 点を投げ, 半径 1 の円の内側に入った点を数える。
     点 i は乱数列 i の 0,1 番目を x,y 座標に使う。 */
  // TODO: 円内に入った点数を reduction(+:count) で集計して π を求めよ.
  // BEGIN ANSWER
#pragma omp parallel for reduction(+:count)
  // END ANSWER
  for (long i = 0; i < n; i++) {
    double x = draw_rand01(i, 0);
    double y = draw_rand01(i, 1);
    if (x * x + y * y < 1.0) count++;
  }
  double pi = 4.0 * (double)count / (double)n;
  printf("count = %ld / %ld\n", count, n);
  printf("pi ~= %.6f\n", pi);
  return 0;
}
