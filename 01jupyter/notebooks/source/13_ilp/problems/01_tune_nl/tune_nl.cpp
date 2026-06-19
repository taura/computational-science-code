#include <cstdio>
#include <cstdlib>
#include <sys/time.h>

enum { nl = 8 };   /* ★ ベクトル長. 1,2,4,8,16,32 と変えて再コンパイルし最良を探す */
typedef double doublev __attribute__((vector_size(sizeof(double) * nl)));

static double sec() {
  struct timeval t; gettimeofday(&t, 0);
  return t.tv_sec + t.tv_usec * 1e-6;
}

int main(int argc, char ** argv) {
  /* m 本の独立な漸化式 t = 0.99*t + 1 を, 各 n 回進める. m は nl の倍数. */
  long m = (1 < argc ? atol(argv[1]) : nl);
  long n = (2 < argc ? atol(argv[2]) : 100L * 1000 * 1000);
  double s = 0.0;
  double t0 = sec();
  for (long g = 0; g < m; g += nl) {
    doublev t;
    for (int k = 0; k < nl; k++) t[k] = 1.0;
    /* nl 本を1命令でまとめて進める (命令レベル並列) */
    for (long j = 0; j < n; j++) t = 0.99 * t + 1.0;
    for (int k = 0; k < nl; k++) s += t[k];
  }
  double dt = sec() - t0;
  double flops = 2.0 * (double)m * (double)n;
  printf("nl=%d, m=%ld, n=%ld : %.3f GFLOPS (s=%f)\n", nl, m, n, flops / dt * 1e-9, s);
  return 0;
}
