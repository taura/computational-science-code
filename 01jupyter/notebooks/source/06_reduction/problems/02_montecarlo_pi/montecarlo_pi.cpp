#include <cstdint>
#include <cstdio>
#include <cstdlib>

/* 反復番号 i から決まる擬似乱数 (splitmix64) を [0,1) の double で返す.
   毎回 i から計算するので, スレッドごとの状態を持たず並列化しやすい. */
static inline double lcg01(uint64_t i) {
  uint64_t x = (i + 1) * 6364136223846793005ULL + 1442695040888963407ULL;
  /* よくかき混ぜる */
  x ^= x >> 30; x *= 0xbf58476d1ce4e5b9ULL;
  x ^= x >> 27; x *= 0x94d049bb133111ebULL;
  x ^= x >> 31;
  /* 上位 53 bit を [0,1) の double に */
  return (double)(x >> 11) * (1.0 / 9007199254740992.0);
}

int main(int argc, char ** argv) {
  long n = (1 < argc ? atol(argv[1]) : 100L * 1000L * 1000L);
  long count = 0;               /* 単位円の 1/4 の内側に入った点数 */
  printf("n = %ld\n", n);
  /* 単位正方形 [0,1)x[0,1) に n 点を投げ, 半径 1 の円の内側に入った点を数える. */
  // TODO: 円内に入った点数を reduction(+:count) で集計して π を求めよ.
  for (long i = 0; i < n; i++) {
    double x = lcg01(2 * i);
    double y = lcg01(2 * i + 1);
    if (x * x + y * y < 1.0) count++;
  }
  double pi = 4.0 * (double)count / (double)n;
  printf("count = %ld / %ld\n", count, n);
  printf("pi ~= %.6f\n", pi);
  return 0;
}
