#include <cstdio>
#include <omp.h>

int main() {
  const int n = 100;
  double a[n];
  for (int i = 0; i < n; i++) {
    a[i] = i + 1;
  }
  // TODO: 下のブロックの直前に #pragma omp parallel を1行追加し, 各スレッドが自分の担当範囲を計算するようにせよ.
  // BEGIN ANSWER
#pragma omp parallel
  // END ANSWER
  {
    int tid = omp_get_thread_num();
    int nt  = omp_get_num_threads();
    int lo  = tid * n / nt;
    int hi  = (tid + 1) * n / nt;
    double s = 0.0;
    for (int i = lo; i < hi; i++) {
      s += a[i];
    }
    printf("thread %d of %d: range [%d, %d), partial sum = %f\n",
           tid, nt, lo, hi, s);
  }
  return 0;
}
