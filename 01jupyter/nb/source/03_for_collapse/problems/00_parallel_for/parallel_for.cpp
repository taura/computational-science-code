#include <cstdio>
#include <omp.h>

int main() {
  const int n = 8;
  int a[n];
  // BEGIN ANSWER: 下の for ループの直前に #pragma omp parallel for を1行追加し, 繰り返しを複数のスレッドに分担させよ.
#pragma omp parallel for
  // END ANSWER
  for (int i = 0; i < n; i++) {
    a[i] = i * i;
    printf("a[%d] = %d\t(thread %d)\n", i, a[i], omp_get_thread_num());
  }
  return 0;
}
