#include <cstdio>
#include <cstdlib>
#include <omp.h>

int main(int argc, char **argv) {
  // 全体で投げる点の数 (コマンドライン引数, 既定 4,000,000)
  long N = (argc > 1) ? atol(argv[1]) : 4000000L;
  // BEGIN ANSWER: 下のブロックの直前に #pragma omp parallel を1行追加し, 各スレッドが自分の担当分の点を投げて, 自分の π 推定値を表示するようにせよ.
#pragma omp parallel
  // END ANSWER
  {
    int tid = omp_get_thread_num();
    int nt  = omp_get_num_threads();
    // このスレッドの担当する点数 (N を T スレッドで分割)
    long lo = tid * N / nt;
    long hi = (tid + 1) * N / nt;
    long my_n = hi - lo;
    // スレッドごとに異なる乱数種 (rand_r はスレッド安全)
    unsigned int seed = tid + 1;
    long hits = 0;
    for (long i = 0; i < my_n; i++) {
      double x = rand_r(&seed) / (double)RAND_MAX;
      double y = rand_r(&seed) / (double)RAND_MAX;
      if (x * x + y * y < 1.0) {
        hits++;
      }
    }
    // 単位正方形に対する 1/4 円の面積比 = π/4
    double pi = (my_n > 0) ? 4.0 * hits / my_n : 0.0;
    printf("thread %d of %d: %ld points, pi estimate = %f\n",
           tid, nt, my_n, pi);
  }
  return 0;
}
