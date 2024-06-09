#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <x86intrin.h>

float mat_vec(long n, float A[n][n], float x[n], float y[n]) {
#pragma omp parallel for
  for (long i = 0; i < n; i++) {
    float s = 0;
    for (long k = 0; k < n; k++) {
      s += A[i][k] * x[k];
    }
    y[i] = s;
  }
}

void init_mat(long n, float A[n][n], float v) {
#pragma omp parallel for
  for (long i = 0; i < n; i++) {
    for (long k = 0; k < n; k++) {
      A[i][k] = v;
    }
  }
}

void init_vec(long n, float x[n], float v) {
  for (long k = 0; k < n; k++) {
    x[k] = v;
  }
}

void check_vec(long n, float x[n], long v) {
  for (long k = 0; k < n; k++) {
    assert(x[k] == n);
  }
}

int main(int argc, char ** argv) {
  long n      = (argc > 1 ? atol(argv[1]) : 1000);
  long repeat = (argc > 2 ? atol(argv[2]) : 10);
  float * A = (float *)malloc(sizeof(float) * n * n);
  float * x = (float *)malloc(sizeof(float) * n);
  float * y = (float *)malloc(sizeof(float) * n);
  init_mat(n, (float(*)[n])A, 1);
  init_vec(n, x, 1);
  long long t0 = _rdtsc();
  for (long r = 0; r < repeat; r++) {
    mat_vec(n, (float(*)[n])A, x, y);
  }
  long long t1 = _rdtsc();
  printf("%lld clocks\n", t1 - t0);
  check_vec(n, y, n);
  return 0;
}
