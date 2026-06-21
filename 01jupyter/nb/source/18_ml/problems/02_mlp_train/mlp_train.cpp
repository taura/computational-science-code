#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <omp.h>

/* 多層パーセプトロン (MLP) を自分で学習させ, 本物の MNIST 手書き数字を分類する。
   ネットワーク: 入力 784 -> 隠れ層 HID=128 (ReLU) -> 出力 10クラス。

   ミニバッチ (m 枚) を「行列」として一度に流す:
     forward:  H = ReLU(X W1^T + b1),  P = softmax(H W2^T + b2)
       X:[m,IN]  H:[m,HID]  P:[m,OUT]
     backward: dO = P - onehot(y),
       gW2 = dO^T H,  gb2 = Σ_行 dO,
       dH  = (dO W2)・[H>0],  gW1 = dH^T X,  gb1 = Σ_行 dH
     更新:     W -= (lr/m) * gW
   勾配 (gW2 等) は「行列積」で求まり, サンプル(バッチ)方向の和は行列積の内側の
   縮約になる。よって並列化は各行列積の独立な出力方向の単純な parallel for でよく,
   配列 reduction は不要 (損失・正解数のスカラ集計だけ reduction を使う)。

   ネットワークの大きさは定数なので, パラメータ・バッチ・中間行列・勾配をすべて
   固定サイズ配列で1つの構造体 Net にまとめる。Net は内部にポインタを持たないので
   GPU 発展では map(to: net) でまるごと送れる。 */

static const int IN = 784, HID = 128, OUT = 10;
static const int MAX_BATCH = 1000;       /* ミニバッチの最大サイズ */

struct Net {
  double W1[HID][IN], b1[HID], W2[OUT][HID], b2[OUT];  /* パラメータ */
  double X[MAX_BATCH][IN];  int y[MAX_BATCH];          /* 現在のバッチ (入力とラベル) */
  double H[MAX_BATCH][HID], P[MAX_BATCH][OUT];         /* 中間 (forward): 隠れ層, 出力確率 */
  double dO[MAX_BATCH][OUT], dH[MAX_BATCH][HID];       /* 中間 (backward): 出力誤差, 隠れ誤差 */
  double gW1[HID][IN], gb1[HID], gW2[OUT][HID], gb2[OUT];   /* 勾配 */
};

/* ====================== .npy 入出力 ====================== */
/* .npy を読み, 任意の数値型を double として dst に直接書き込む (C順)。次元数 ndim は
   呼び出し側が指定し, ファイルの次元が違えばエラー。形を shape[0..ndim-1] に返す。 */
static void read_npy(const char * path, double * dst, long * shape, int ndim) {
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
  long n = s0;
  if (ndim == 2) { shape[1] = s1; n *= s1; }
  delete[] hdr;
  if (dst == nullptr) { fclose(f); return; }   /* 形だけ知りたいとき (dst=nullptr) */

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

/* double 配列を float64 の .npy として書き出す (C順, s1=0 なら1次元) */
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

/* ====================== 行列演算・活性化 ====================== */
/* 行列ベクトル積 + バイアス: y = W x + b  (W は R×C, サイズは型から自動で決まる) */
template <int R, int C>
static void matvec(const double (&W)[R][C], const double * x,
                   const double (&b)[R], double (&y)[R]) {
  for (int i = 0; i < R; i++) {
    double s = b[i];
    for (int j = 0; j < C; j++) s += W[i][j] * x[j];
    y[i] = s;
  }
}

static void relu_inplace(double * v, int n) {
  for (int i = 0; i < n; i++) if (v[i] < 0.0) v[i] = 0.0;
}

static void softmax_inplace(double * v, int n) {
  double mx = v[0];
  for (int i = 1; i < n; i++) if (v[i] > mx) mx = v[i];
  double s = 0.0;
  for (int i = 0; i < n; i++) { v[i] = exp(v[i] - mx); s += v[i]; }
  for (int i = 0; i < n; i++) v[i] /= s;
}

static int argmax(const double * v, int n) {
  int best = 0;
  for (int i = 1; i < n; i++) if (v[i] > v[best]) best = i;
  return best;
}

/* 状態を持たない乱数 (初期値生成用): (seed,k) から [0,1)。 */
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

/* ====================== 学習の心臓部 (バッチ m 枚を行列で処理) ====================== */
struct LossCorrect { double loss; long correct; };

/* forward: 各サンプル行 i は独立。H[i], P[i] を埋め, 損失と正解数を集計して返す。 */
static LossCorrect forward(Net & net, int m) {
  double loss = 0.0; long correct = 0;
  // TODO: 各サンプル行 i の forward を並列化する (行ごと独立, 損失・正解数はスカラ集計)。
  // BEGIN ANSWER
#pragma omp parallel for reduction(+:loss,correct)
  // END ANSWER
  for (int i = 0; i < m; i++) {
    matvec(net.W1, net.X[i], net.b1, net.H[i]); relu_inplace(net.H[i], HID);
    matvec(net.W2, net.H[i], net.b2, net.P[i]); softmax_inplace(net.P[i], OUT);
    loss -= log(net.P[i][net.y[i]] + 1e-12);
    if (argmax(net.P[i], OUT) == net.y[i]) correct++;
  }
  return {loss, correct};
}

/* backward: 勾配を行列積で求める。各行列積の出力方向は独立なので reduction は不要。 */
static void backward(Net & net, int m) {
  /* 出力誤差 dO = P - onehot(y) : 行 i ごと独立 */
  // TODO: dO を計算するループを並列化する (行 i ごと独立)。
  // BEGIN ANSWER
#pragma omp parallel for
  // END ANSWER
  for (int i = 0; i < m; i++)
    for (int c = 0; c < OUT; c++) net.dO[i][c] = net.P[i][c] - (c == net.y[i] ? 1.0 : 0.0);

  /* gW2 = dO^T H, gb2 = Σ_行 dO : 出力 c ごと独立 (バッチ i は内側の和) */
  // TODO: gW2,gb2 を計算するループを並列化する (出力 c ごと独立)。
  // BEGIN ANSWER
#pragma omp parallel for
  // END ANSWER
  for (int c = 0; c < OUT; c++) {
    double sb = 0.0;
    for (int i = 0; i < m; i++) sb += net.dO[i][c];
    net.gb2[c] = sb;
    for (int k = 0; k < HID; k++) {
      double s = 0.0;
      for (int i = 0; i < m; i++) s += net.dO[i][c] * net.H[i][k];
      net.gW2[c][k] = s;
    }
  }

  /* 隠れ誤差 dH = (dO W2)・[H>0] : 行 i ごと独立 */
  // TODO: dH を計算するループを並列化する (行 i ごと独立)。
  // BEGIN ANSWER
#pragma omp parallel for
  // END ANSWER
  for (int i = 0; i < m; i++)
    for (int k = 0; k < HID; k++) {
      double s = 0.0;
      for (int c = 0; c < OUT; c++) s += net.dO[i][c] * net.W2[c][k];
      net.dH[i][k] = (net.H[i][k] > 0.0) ? s : 0.0;
    }

  /* gW1 = dH^T X, gb1 = Σ_行 dH : 隠れ k ごと独立 */
  // TODO: gW1,gb1 を計算するループを並列化する (隠れ k ごと独立)。
  // BEGIN ANSWER
#pragma omp parallel for
  // END ANSWER
  for (int k = 0; k < HID; k++) {
    double sb = 0.0;
    for (int i = 0; i < m; i++) sb += net.dH[i][k];
    net.gb1[k] = sb;
    for (int j = 0; j < IN; j++) {
      double s = 0.0;
      for (int i = 0; i < m; i++) s += net.dH[i][k] * net.X[i][j];
      net.gW1[k][j] = s;
    }
  }
}

/* 勾配降下の1ステップ: W -= (lr/m) gW (バッチ平均の勾配で降下) */
static void sgd_update(Net & net, int m, double lr) {
  double sc = lr / (double)m;
  for (long k = 0; k < (long)HID * IN; k++)  (&net.W1[0][0])[k] -= sc * (&net.gW1[0][0])[k];
  for (int k = 0; k < HID; k++)              net.b1[k] -= sc * net.gb1[k];
  for (long k = 0; k < (long)OUT * HID; k++) (&net.W2[0][0])[k] -= sc * (&net.gW2[0][0])[k];
  for (int c = 0; c < OUT; c++)              net.b2[c] -= sc * net.gb2[c];
}

/* ====================== main ====================== */
static Net net;          /* バッチ・中間行列も含み大きいので静的領域に置く */

int main(int argc, char ** argv) {
  int    E  = (argc > 1 ? atoi(argv[1]) : 20);    /* エポック数 */
  double lr = (argc > 2 ? atof(argv[2]) : 0.1);   /* 学習率 */
  int    BS = (argc > 3 ? atoi(argv[3]) : 100);   /* ミニバッチサイズ */
  if (BS > MAX_BATCH) { printf("BS は %d 以下にしてください\n", MAX_BATCH); return 1; }

  /* 訓練データ全体 (枚数 N は可変) をヒープに読む。まず形だけ見て N を得てから確保する。 */
  long sh[2];
  read_npy("data/x_train.npy", nullptr, sh, 2);   /* dst=nullptr で形だけ取得 */
  long N = sh[0];
  double * Xall = new double[N * IN];
  read_npy("data/x_train.npy", Xall, sh, 2);
  for (long i = 0; i < N * IN; i++) Xall[i] /= 255.0;  /* 0..255 -> 0..1 */
  double * yd = new double[N];
  read_npy("data/y_train.npy", yd, sh, 1);
  int * yall = new int[N];
  for (long i = 0; i < N; i++) yall[i] = (int)yd[i];
  delete[] yd;

  /* パラメータ初期化 (He 初期化, バイアスは 0) */
  init_he(&net.W1[0][0], (long)HID * IN, IN, 1);
  init_he(&net.W2[0][0], (long)OUT * HID, HID, 2);
  for (int k = 0; k < HID; k++) net.b1[k] = 0.0;
  for (int c = 0; c < OUT; c++) net.b2[c] = 0.0;

  double loss = 0.0; long correct = 0;
  double t0 = omp_get_wtime();
  for (int ep = 0; ep < E; ep++) {
    loss = 0.0; correct = 0;
    for (long b0 = 0; b0 < N; b0 += BS) {
      int m = (int)((b0 + BS < N) ? BS : N - b0);
      /* 今のバッチを net.X, net.y にコピー */
      for (int i = 0; i < m; i++) {
        for (int j = 0; j < IN; j++) net.X[i][j] = Xall[(b0 + i) * IN + j];
        net.y[i] = yall[b0 + i];
      }
      LossCorrect r = forward(net, m);
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

  write_npy("data/W1.npy", &net.W1[0][0], HID, IN);
  write_npy("data/b1.npy", net.b1, HID, 0);
  write_npy("data/W2.npy", &net.W2[0][0], OUT, HID);
  write_npy("data/b2.npy", net.b2, OUT, 0);
  printf("重みを data/W1.npy, b1.npy, W2.npy, b2.npy に保存しました\n");

  delete[] Xall; delete[] yall;
  return 0;
}
