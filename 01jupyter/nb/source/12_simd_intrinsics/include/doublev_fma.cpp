#include <cstdio>

typedef double doublev __attribute__((vector_size(64)));

/* a, b, c はいずれも double 8 つ分のベクトル型.
   a*b+c も要素ごとの演算としてベクトルのまま計算される */
doublev doublev_fma(doublev a, doublev b, doublev c) {
  return a * b + c;
}
