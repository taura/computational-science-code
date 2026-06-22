#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <omp.h>

/* 多層パーセプトロン (MLP) を自分で学習させ, 本物の MNIST 手書き数字を分類する。
   ネットワーク: 入力 784 -> 隠れ層 HID=128 (ReLU) -> 出力 10クラス。

   ミニバッチ (m 枚) を「行列」としてまとめて流す。各ステップはバッチ中の全サンプルを
   一度に処理する行列演算で, forward / backward は下のプリミティブを呼ぶだけ:
     forward:  H = matmul_bias(W1,X,b1); relu(H)
               P = matmul_bias(W2,H,b2); softmax(P)
     backward: dO = out_grad(P,y)                  (= P - onehot(y))
               gW2,gb2 = grad_weight(dO,H)         (= dO^T H, 列和)
               dH = matmul_back(dO,W2); relu_mask(dH,H)
               gW1,gb1 = grad_weight(dH,X)
   行列積 (matmul_bias / matmul_back / grad_weight) を活性化 (relu/softmax/…) から分け,
   重い行列積を並列化する (活性化は要素ごとで軽いので逐次)。
     更新:     W -= (lr/m) gW
   勾配は行列積で求まり, サンプル(バッチ)方向の和は行列積の内側の縮約になる。よって
   並列化は各プリミティブの独立な出力方向ループの parallel for だけでよく, 勾配への
   配列 reduction は不要 (損失・正解数のスカラ集計だけ reduction を使う)。

   パラメータ・バッチ・中間行列・勾配を固定サイズ配列で1つの構造体 Net にまとめる。
   Net は内部にポインタを持たないので, GPU 発展では map(to: net) でまるごと送れる。 */

static const int IN = 784, HID = 128, OUT = 10;
static const int MAX_BATCH = 1000;       /* ミニバッチの最大サイズ */

struct Net {
  double W1[HID][IN], b1[HID], W2[OUT][HID], b2[OUT];  /* パラメータ */
  double X[MAX_BATCH][IN]; double y[MAX_BATCH];        /* 現在のバッチ (入力とラベル) */
  double H[MAX_BATCH][HID], P[MAX_BATCH][OUT];         /* 中間 (forward) */
  double dO[MAX_BATCH][OUT], dH[MAX_BATCH][HID];       /* 中間 (backward) */
  double gW1[HID][IN], gb1[HID], gW2[OUT][HID], gb2[OUT];   /* 勾配 */
};

/* ====================== .npy 入出力 ====================== */
static void read_npy(const char * path, double * dst, long * shape, int ndim, long maxrows = -1) {
  FILE * f = fopen(path, "rb");
  if (!f) { printf("%s が開けません\n", path); exit(1); }
  unsigned char magic[10];
  if (fread(magic, 1, 10, f) != 10 || memcmp(magic, "\x93NUMPY", 6) != 0) {
    printf("%s は .npy ではありません\n", path); exit(1);
  }
  int hlen = magic[8] | (magic[9] << 8);
  char * hdr = new char[hlen + 1];
  fread(hdr, 1, hlen, f); hdr[hlen] = '\0';
  char descr[8] = {0};
  { char * q = strstr(hdr, "descr"); q = strchr(q, ':'); q = strchr(q, '\'') + 1;
    int i = 0; while (*q != '\'' && i < 7) descr[i++] = *q++; descr[i] = '\0'; }
  long s0 = 1, s1 = 1; int file_ndim;
  char * sp = strstr(hdr, "shape"); sp = strchr(sp, '(') + 1;
  s0 = atol(sp);
  char * comma = strchr(sp, ',');
  char * after = comma + 1; while (*after == ' ') after++;
  if (*after != ')') { s1 = atol(after); file_ndim = 2; } else { file_ndim = 1; }
  if (file_ndim != ndim) {
    printf("%s: %d 次元を期待しましたが %d 次元でした\n", path, ndim, file_ndim); exit(1);
  }
  shape[0] = s0;
  if (ndim == 2) shape[1] = s1;
  delete[] hdr;
  if (dst == nullptr) { fclose(f); return; }              /* 形だけ (peek) */

  long rows = (maxrows >= 0 && s0 > maxrows) ? maxrows : s0;
  long n = rows * (ndim == 2 ? s1 : 1);
  if (!strcmp(descr, "<f8")) {
    fread(dst, sizeof(double), n, f);
  } else if (!strcmp(descr, "<f4")) {
    float * t = new float[n]; fread(t, sizeof(float), n, f);
    for (long i = 0; i < n; i++) { dst[i] = t[i]; } delete[] t;
  } else if (!strcmp(descr, "|u1")) {
    unsigned char * t = new unsigned char[n]; fread(t, 1, n, f);
    for (long i = 0; i < n; i++) { dst[i] = t[i]; } delete[] t;
  } else if (!strcmp(descr, "<i4")) {
    int * t = new int[n]; fread(t, sizeof(int), n, f);
    for (long i = 0; i < n; i++) { dst[i] = t[i]; } delete[] t;
  } else if (!strcmp(descr, "<i8")) {
    long long * t = new long long[n]; fread(t, sizeof(long long), n, f);
    for (long i = 0; i < n; i++) { dst[i] = (double)t[i]; } delete[] t;
  } else {
    printf("%s: 未対応の dtype %s\n", path, descr); exit(1);
  }
  fclose(f);
}

static void write_npy(const char * path, const double * data, long s0, long s1) {
  FILE * f = fopen(path, "wb");
  if (!f) { printf("%s が書けません\n", path); exit(1); }
  char shape[64];
  if (s1 > 0) snprintf(shape, sizeof(shape), "(%ld, %ld)", s0, s1);
  else        snprintf(shape, sizeof(shape), "(%ld,)", s0);
  char dict[128];
  int dn = snprintf(dict, sizeof(dict),
                    "{'descr': '<f8', 'fortran_order': False, 'shape': %s, }", shape);
  int total = 10 + dn + 1;
  int pad = (64 - (total % 64)) % 64;
  int hlen = dn + 1 + pad;
  unsigned char head[10] = {0x93,'N','U','M','P','Y',1,0,
                            (unsigned char)(hlen & 0xff),(unsigned char)(hlen >> 8)};
  fwrite(head, 1, 10, f);
  fwrite(dict, 1, dn, f);
  for (int i = 0; i < pad; i++) fputc(' ', f);
  fputc('\n', f);
  long n = s0 * (s1 > 0 ? s1 : 1);
  fwrite(data, sizeof(double), n, f);
  fclose(f);
}

/* ====================== バッチ行列演算 (各ステップが m 枚を一度に処理) ====================== */
/* 行列積 + バイアス: Y = X W^T + b   (W:R×C, X:m×C, Y:m×R)。行 i (サンプル) ごとに独立。
   これが「AI の計算の中身」である行列積。重いのでここを並列化する。 */
template <int R, int C>
static void matmul_bias(const double (&W)[R][C], const double X[][C],
                   const double (&b)[R], double Y[][R], int m) {
  // TODO: 行 i (サンプル) のループを並列化する (各行は独立)。
  // BEGIN ANSWER
#pragma omp parallel for
  // END ANSWER
  for (int i = 0; i < m; i++)
    for (int k = 0; k < R; k++) {
      double s = b[k];
      for (int j = 0; j < C; j++) s += W[k][j] * X[i][j];
      Y[i][k] = s;
    }
}

/* 逆向きの行列積 (バイアス無し): dX = dY W   (dY:m×R, W:R×C, dX:m×C)。行 i ごとに独立。 */
template <int R, int C>
static void matmul_back(const double dY[][R], const double (&W)[R][C], double dX[][C], int m) {
  // TODO: 行 i のループを並列化する (各行は独立)。
  // BEGIN ANSWER
#pragma omp parallel for
  // END ANSWER
  for (int i = 0; i < m; i++)
    for (int j = 0; j < C; j++) {
      double s = 0.0;
      for (int k = 0; k < R; k++) s += dY[i][k] * W[k][j];
      dX[i][j] = s;
    }
}

/* 活性化・要素ごとの軽い処理 (逐次でよい) */
template <int N> static void relu(double Y[][N], int m) {
  for (int i = 0; i < m; i++)
    for (int k = 0; k < N; k++) if (Y[i][k] < 0.0) Y[i][k] = 0.0;
}
template <int N> static void softmax(double Y[][N], int m) {
  for (int i = 0; i < m; i++) {
    double mx = Y[i][0];
    for (int k = 1; k < N; k++) if (Y[i][k] > mx) mx = Y[i][k];
    double s = 0.0;
    for (int k = 0; k < N; k++) { Y[i][k] = exp(Y[i][k] - mx); s += Y[i][k]; }
    for (int k = 0; k < N; k++) Y[i][k] /= s;
  }
}
/* 出力誤差 dO = P - onehot(y) */
template <int N> static void out_grad(const double P[][N], const double * y, double dO[][N], int m) {
  for (int i = 0; i < m; i++)
    for (int c = 0; c < N; c++) dO[i][c] = P[i][c] - (c == (int)y[i] ? 1.0 : 0.0);
}
/* ReLU の逆伝播マスク: dX[i][k] を Href[i][k]<=0 のところで 0 にする (in-place) */
template <int N> static void relu_mask(double dX[][N], const double Href[][N], int m) {
  for (int i = 0; i < m; i++)
    for (int k = 0; k < N; k++) if (Href[i][k] <= 0.0) dX[i][k] = 0.0;
}

/* 重み勾配: G = A^T B (バッチ和), gb = 列和(A)。A は m×R, B は m×C, G は R×C, gb は R。
   出力 k ごとに独立 (バッチ i は内側の和)。 */
template <int R, int C>
static void grad_weight(const double A[][R], const double B[][C],
                        double (&G)[R][C], double (&gb)[R], int m) {
  // TODO: 出力 k のループを並列化する (k ごと独立, バッチ i は内側の和)。
  // BEGIN ANSWER
#pragma omp parallel for
  // END ANSWER
  for (int k = 0; k < R; k++) {
    double sb = 0.0;
    for (int i = 0; i < m; i++) sb += A[i][k];
    gb[k] = sb;
    for (int j = 0; j < C; j++) {
      double s = 0.0;
      for (int i = 0; i < m; i++) s += A[i][k] * B[i][j];
      G[k][j] = s;
    }
  }
}

static int argmax(const double * v, int n) {
  int best = 0;
  for (int i = 1; i < n; i++) if (v[i] > v[best]) best = i;
  return best;
}

/* バッチの損失と正解数を集計する (行ごと独立, スカラ reduction) */
struct LossCorrect { double loss; long correct; };
static LossCorrect eval(Net & net, int m) {
  double loss = 0.0; long correct = 0;
  // TODO: 行 i のループを並列化して損失・正解数を集計する (スカラ reduction)。
  // BEGIN ANSWER
#pragma omp parallel for reduction(+:loss,correct)
  // END ANSWER
  for (int i = 0; i < m; i++) {
    loss -= log(net.P[i][(int)net.y[i]] + 1e-12);
    if (argmax(net.P[i], OUT) == (int)net.y[i]) correct++;
  }
  return {loss, correct};
}

/* ====================== forward / backward / 更新 (プリミティブを呼ぶだけ) ====================== */
static void forward(Net & net, int m) {
  matmul_bias(net.W1, net.X, net.b1, net.H, m); relu(net.H, m);    /* H = ReLU(W1 X + b1)   */
  matmul_bias(net.W2, net.H, net.b2, net.P, m); softmax(net.P, m); /* P = softmax(W2 H + b2) */
}

static void backward(Net & net, int m) {
  out_grad   (net.P, net.y, net.dO, m);            /* dO = P - onehot(y)            */
  grad_weight(net.dO, net.H, net.gW2, net.gb2, m); /* gW2 = dO^T H, gb2 = Σ dO      */
  matmul_back(net.dO, net.W2, net.dH, m);          /* dH = dO W2                    */
  relu_mask  (net.dH, net.H, m);                   /* dH を [H>0] でマスク          */
  grad_weight(net.dH, net.X, net.gW1, net.gb1, m); /* gW1 = dH^T X, gb1 = Σ dH      */
}

static void sgd_update(Net & net, int m, double lr) {
  double sc = lr / (double)m;
  for (long k = 0; k < (long)HID * IN; k++)  (&net.W1[0][0])[k] -= sc * (&net.gW1[0][0])[k];
  for (int k = 0; k < HID; k++)              net.b1[k] -= sc * net.gb1[k];
  for (long k = 0; k < (long)OUT * HID; k++) (&net.W2[0][0])[k] -= sc * (&net.gW2[0][0])[k];
  for (int c = 0; c < OUT; c++)              net.b2[c] -= sc * net.gb2[c];
}

/* ====================== 初期化・データ読み込み ====================== */
static inline double draw_rand01(long long seed, long long k) {
  const long long M = 2147483647LL;
  long long x = ((seed % M) * 2654435761LL + (k % M) + 1) % M;
  x = ((x ^ (x >> 16)) * 1812433253LL) % M;
  x = ((x ^ (x >> 13)) * 1664525LL)    % M;
  x =  (x ^ (x >> 16)) % M;
  return (double)x / (double)M;
}

static void init_he(double * w, long total, int n_in, long long salt) {
  double scale = sqrt(2.0 / n_in) * 2.0;
  for (long k = 0; k < total; k++) w[k] = (draw_rand01(k, salt) - 0.5) * scale;
}

static void zero(double * v, long n) { for (long i = 0; i < n; i++) v[i] = 0.0; }

/* 全データの b0..b0+m-1 の m 枚を net.X, net.y にコピーする */
static void load_batch(Net & net, const double * Xall, const double * yall, long b0, int m) {
  for (int i = 0; i < m; i++) {
    for (int j = 0; j < IN; j++) net.X[i][j] = Xall[(b0 + i) * IN + j];
    net.y[i] = yall[b0 + i];
  }
}

/* 訓練データ (画像と正解ラベル) を .npy から読み込む。画像は 0..1 に正規化し,
   X, y を新たに確保して返し, 枚数 N を返す (枚数は可変なのでまず形を見てから確保)。 */
static long load_dataset(const char * xpath, const char * ypath, double *& X, double *& y) {
  long sh[2];
  read_npy(xpath, nullptr, sh, 2);
  long N = sh[0];
  X = new double[N * IN];
  read_npy(xpath, X, sh, 2);
  for (long i = 0; i < N * IN; i++) X[i] /= 255.0;
  y = new double[N];
  read_npy(ypath, y, sh, 1);
  return N;
}

/* ====================== main ====================== */
static Net net;          /* バッチ・中間行列も含み大きいので静的領域に置く */

int main(int argc, char ** argv) {
  int    E  = (argc > 1 ? atoi(argv[1]) : 20);    /* エポック数 */
  double lr = (argc > 2 ? atof(argv[2]) : 0.1);   /* 学習率 */
  int    BS = (argc > 3 ? atoi(argv[3]) : 100);   /* ミニバッチサイズ */
  if (BS > MAX_BATCH) { printf("BS は %d 以下にしてください\n", MAX_BATCH); return 1; }

  double * Xall; double * yall;
  long N = load_dataset("data/x_train.npy", "data/y_train.npy", Xall, yall);

  init_he(&net.W1[0][0], (long)HID * IN, IN, 1);
  init_he(&net.W2[0][0], (long)OUT * HID, HID, 2);
  zero(net.b1, HID); zero(net.b2, OUT);

  double loss = 0.0; long correct = 0;
  double t0 = omp_get_wtime();
  for (int ep = 0; ep < E; ep++) {
    loss = 0.0; correct = 0;
    for (long b0 = 0; b0 < N; b0 += BS) {
      int m = (int)((b0 + BS < N) ? BS : N - b0);
      load_batch(net, Xall, yall, b0, m);
      forward(net, m);
      LossCorrect r = eval(net, m);
      loss += r.loss; correct += r.correct;
      backward(net, m);
      sgd_update(net, m, lr);
    }
    loss /= (double)N;
    if (ep % 5 == 0 || ep == E - 1)
      printf("epoch %4d: loss=%.4f, train acc=%.2f%%\n", ep, loss, 100.0 * correct / N);
  }
  double elapsed = omp_get_wtime() - t0;

  printf("最終: N=%ld, HID=%d, epochs=%d, loss=%.4f, train acc=%.2f%%\n",
         N, HID, E, loss, 100.0 * correct / N);
  printf("elapsed = %.3f sec\n", elapsed);

  write_npy("weights/W1.npy", &net.W1[0][0], HID, IN);
  write_npy("weights/b1.npy", net.b1, HID, 0);
  write_npy("weights/W2.npy", &net.W2[0][0], OUT, HID);
  write_npy("weights/b2.npy", net.b2, OUT, 0);
  printf("重みを weights/W1.npy, b1.npy, W2.npy, b2.npy に保存しました\n");

  delete[] Xall; delete[] yall;
  return 0;
}
