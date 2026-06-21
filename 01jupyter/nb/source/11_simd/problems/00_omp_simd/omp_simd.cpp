/* y[i] = a * x[i] + y[i] (saxpy/axpy) を n 要素について行う */
void saxpy(long n, double a, double * x, double * y) {
  // TODO: 下の for ループの直前に #pragma omp simd を1行追加し, このループをSIMD化せよ.
  // BEGIN ANSWER
  #pragma omp simd
  // END ANSWER
  for (long i = 0; i < n; i++) {
    y[i] = a * x[i] + y[i];
  }
}
