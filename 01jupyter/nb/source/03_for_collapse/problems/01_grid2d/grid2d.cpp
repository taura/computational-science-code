#include <cstdio>
#include <omp.h>

int main() {
  const int N = 4;
  double a[N][N];
  // TODO: 下の二重ループの直前に #pragma omp parallel for collapse(2) を1行追加し, 二重ループ全体を複数スレッドに分担させよ.
  // BEGIN ANSWER
#pragma omp parallel for collapse(2)
  // END ANSWER
  for (int i = 0; i < N; i++) {
    for (int j = 0; j < N; j++) {
      a[i][j] = i * 10 + j;
      printf("a[%d][%d] = %g  (thread %d)\n",
             i, j, a[i][j], omp_get_thread_num());
    }
  }
  return 0;
}
