#include <cstdio>
#include <omp.h>

/* 仕事量が引数 k に比例するダミー計算 (k に比例した回数だけ加算する) */
double work(long k) {
  double s = 0.0;
  for (long j = 0; j < k; j++) {
    s += 1.0 / (1.0 + j);
  }
  return s;
}

int main() {
  const int n = 2000;
  double total = 0.0;
  // BEGIN ANSWER: 下のループを並列化し, 仕事量が i に比例して不均一なので schedule(dynamic) で負荷を均せ.
#pragma omp parallel for schedule(dynamic) reduction(+:total)
  // END ANSWER
  for (int i = 0; i < n; i++) {
    /* 繰り返し i の仕事量は i に比例して重くなる (アンバランス) */
    total += work((long)i * 100000L);
  }
  printf("total = %f\n", total);
  return 0;
}
