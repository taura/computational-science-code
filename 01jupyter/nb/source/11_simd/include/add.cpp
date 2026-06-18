void add(double *x, double *y, double *z) {
  for (int i = 0; i < 8; i++) {
    z[i] = x[i] + y[i];
  }
}
