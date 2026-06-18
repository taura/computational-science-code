#include <cstdio>

/* double を 8 つ (64 バイト = 512 bit) 束ねたベクトル型 */
typedef double doublev __attribute__((vector_size(64)));

/* 全要素が 0 のベクトルを作る */
doublev zero() {
  doublev z = {0, 0, 0, 0, 0, 0, 0, 0};
  return z;
}
