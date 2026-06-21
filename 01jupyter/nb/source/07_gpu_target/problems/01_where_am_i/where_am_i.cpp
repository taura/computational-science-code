#include <cstdio>
#include <omp.h>

int main() {
  printf("on host: omp_is_initial_device() = %d\n", omp_is_initial_device());
  // TODO: 下のブロックの直前に #pragma omp target を1行追加し, ブロックの中身をデバイス(GPU)上で実行させよ. (表示するだけなので map 節は不要)
  // BEGIN ANSWER
#pragma omp target
  // END ANSWER
  {
    printf("inside target: omp_is_initial_device() = %d\n", omp_is_initial_device());
  }
  return 0;
}
