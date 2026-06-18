void saxpy(int n, double a, double *x, double *y) {
#pragma omp simd
  for (int i = 0; i < n; i++) {
    y[i] = a * x[i] + y[i];
  }
}
