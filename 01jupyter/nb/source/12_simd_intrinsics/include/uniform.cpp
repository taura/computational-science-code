#include <cstdio>

typedef double doublev __attribute__((vector_size(64)));

/* 全要素が u であるようなベクトルを作る */
doublev uniform(double u) {
  doublev v = {u, u, u, u, u, u, u, u};
  return v;
}
