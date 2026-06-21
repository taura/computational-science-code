#include <cstdio>
#include <omp.h>

int main() {
  // TODO: 下のブロックの直前に #pragma omp parallel を1行追加し, printf を複数のスレッドで実行させよ.
  // BEGIN ANSWER
#pragma omp parallel
  // END ANSWER
  {
    printf("hello from thread %d of %d\n",
           omp_get_thread_num(), omp_get_num_threads());
  }
  return 0;
}
