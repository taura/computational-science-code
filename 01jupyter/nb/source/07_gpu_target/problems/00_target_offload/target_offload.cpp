#include <cstdio>
#include <omp.h>

int main() {
  // TODO: 下のブロックの直前に #pragma omp target を1行追加し, printf をデバイス(GPU)上で実行させよ.
  // BEGIN ANSWER
#pragma omp target
  // END ANSWER
  {
    printf("hello from the device\n");
  }
  return 0;
}
