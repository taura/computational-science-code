#include <cstdio>
#include <cstdlib>

int main(int argc, char **argv) {
  // 画像サイズと最大反復数
  int W = (argc > 1) ? atoi(argv[1]) : 1000;
  int H = (argc > 2) ? atoi(argv[2]) : 1000;
  int maxiter = (argc > 3) ? atoi(argv[3]) : 1000;

  int *cnt = (int *)malloc((size_t)W * H * sizeof(int));

  // 複素平面の描画範囲
  const double xmin = -2.0, xmax = 1.0;
  const double ymin = -1.5, ymax = 1.5;

  // 各ピクセルの脱出反復数を計算する.
  // 内部の点は maxiter まで回るため画素ごとの仕事量が大きく異なる (負荷が不均一).
  // TODO: 下の行 (px ループ) の直前に #pragma omp parallel for schedule(dynamic) を追加せよ. 仕事量が画素ごとに大きく異なるため, dynamic スケジュールが負荷を均す.
  // BEGIN ANSWER
#pragma omp parallel for schedule(dynamic)
  // END ANSWER
  for (int px = 0; px < W * H; px++) {
    int i = px / W;  // 行
    int j = px % W;  // 列
    double cx = xmin + (xmax - xmin) * j / (W - 1);
    double cy = ymin + (ymax - ymin) * i / (H - 1);
    // z = z^2 + c を |z|^2 > 4 か maxiter まで反復 (複素数を手で展開)
    double zr = 0.0, zi = 0.0;
    int it = 0;
    while (it < maxiter && zr * zr + zi * zi <= 4.0) {
      double zr2 = zr * zr - zi * zi + cx;
      double zi2 = 2.0 * zr * zi + cy;
      zr = zr2;
      zi = zi2;
      it++;
    }
    cnt[px] = it;
  }

  // 並列ループの後で総反復数を逐次に集計する (共有変数への足し込みによる競合を避ける)
  long long total = 0;
  for (int px = 0; px < W * H; px++) {
    total += cnt[px];
  }

  printf("W=%d H=%d maxiter=%d\n", W, H, maxiter);
  printf("total iterations = %lld\n", total);
  printf("sample cnt: top-left=%d center=%d bottom-right=%d\n",
         cnt[0], cnt[(H / 2) * W + W / 2], cnt[W * H - 1]);

  free(cnt);
  return 0;
}
