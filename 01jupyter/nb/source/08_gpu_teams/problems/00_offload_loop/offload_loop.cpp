#include <cstdio>
#include <cstdlib>
#include <omp.h>

int main(int argc, char ** argv) {
  long m = (1 < argc ? atol(argv[1]) : 8);
  // BEGIN ANSWER: 下の for 文の直前に #pragma omp target teams distribute parallel for を1行追加し, ループをGPU上の多数のチーム×スレッドで並列実行させよ. (結果を表示するだけなので map 節は不要)
#pragma omp target teams distribute parallel for
  // END ANSWER
  for (long i = 0; i < m; i++) {
    printf("i = %ld  executed by team %d  thread %d\n",
           i, omp_get_team_num(), omp_get_thread_num());
  }
  return 0;
}
