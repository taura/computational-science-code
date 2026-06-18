#include <cstdio>

/* double 8 つ分 (8 × 8 = 64 バイト = 512 bit) のベクトル型 */
typedef double doublev __attribute__((vector_size(64)));

/* a, b, c はいずれも double 8 つ分のベクトル型.
   a*b+c を要素ごとの演算 (fma) としてベクトルのまま計算して返す. */
doublev vector_fma(doublev a, doublev b, doublev c) {
  // BEGIN ANSWER: a * b + c を返す1行を書け.
  return a * b + c;
  // END ANSWER
}
