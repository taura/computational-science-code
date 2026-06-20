#include <cstdio>
#include <cstdlib>
#include <cmath>

/* 試行ごとに独立な乱数生成器 (MINSTD 法)。
   16807*s は 64bit に収まるので桁あふれしない。返り値は [0,N) の整数。 */
static inline int draw(long long * s, int N) {
  *s = (16807LL * *s) % 2147483647LL;
  return (int)(*s % N);
}

/* 1試行: N 種類の景品が等確率で出るとき, 全種類そろうまでに引いた回数。
   collected を 64bit のビットマスクで管理する (N <= 62 を想定)。 */
long one_trial(int N, long long seed) {
  long long s = seed % 2147483647LL;
  if (s <= 0) s += 2147483646LL;
  unsigned long long got = 0;
  unsigned long long full = (N == 64 ? ~0ULL : ((1ULL << N) - 1));
  long draws = 0;
  while (got != full) {
    int k = draw(&s, N);
    got |= (1ULL << k);
    draws++;
  }
  return draws;
}

int main(int argc, char ** argv) {
  int  N = (argc > 1 ? atoi(argv[1]) : 10);          /* 景品の種類数 */
  long T = (argc > 2 ? atol(argv[2]) : 1000000);     /* 試行回数 */
  double sum = 0.0, sumsq = 0.0;

  /* T 回の試行は互いに独立。各試行の引き回数 d を集計する。 */
  // BEGIN ANSWER: 各試行は独立なので #pragma omp parallel for reduction(+:sum,sumsq) で並列化・集計せよ.
#pragma omp parallel for reduction(+:sum,sumsq)
  // END ANSWER
  for (long t = 0; t < T; t++) {
    long d = one_trial(N, (long long)(t + 1) * 2654435761LL + 12345LL);
    sum   += (double)d;
    sumsq += (double)d * (double)d;
  }

  double mean = sum / T;
  double var  = sumsq / T - mean * mean;
  /* 理論期待値 = N * H_N (H_N は調和数) */
  double H = 0.0;
  for (int k = 1; k <= N; k++) H += 1.0 / k;
  printf("N=%d, trials=%ld: 平均 %.3f 回 (理論 %.3f), 標準偏差 %.3f\n",
         N, T, mean, N * H, sqrt(var));
  return 0;
}
