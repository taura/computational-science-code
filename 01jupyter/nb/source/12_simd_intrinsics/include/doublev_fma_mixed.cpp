#include <cstdio>

typedef double doublev __attribute__((vector_size(64)));

/* a はスカラ(double), b, c はベクトル(doublev).
   a は自動的に 8 つに複製(broadcast)されてから演算される */
doublev doublev_fma_mixed(double a, doublev b, doublev c) {
  return a * b + c;
}
