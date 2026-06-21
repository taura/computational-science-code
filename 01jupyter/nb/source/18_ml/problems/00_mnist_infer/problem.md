# 応用問題: MNIST 手書き数字の認識 (行列積 = AI推論)

## 目標

**ニューラルネットワークの推論 (forward) の正体は「行列積 + 活性化関数」** である。これまで何度も並列化してきた行列(ベクトル)積が, そのまま AI の推論計算になっていることを, **本物の MNIST 手書き数字**を認識して体感する。

題材は 2層 MLP:

- 入力 784次元 (28×28 画像を一列に並べたもの) -> 隠れ層 128 (ReLU) -> 出力 10クラス
- `h = ReLU(W1 x + b1)` (128次元), `o = W2 h + b2` (10次元), 予測クラス = `argmax(o)`

`data/` に **学習済みの重み**と **本物のテスト画像** が用意してある:

- `data/mnist_weights.txt` : 学習済みの重み `W1, b1, W2, b2`
- `data/mnist_test.txt` : MNIST テスト画像 1000 枚 (各 784 画素, 0..255) と正解ラベル

(重みは MNIST 訓練データであらかじめ学習させたもの。学習そのものは `02_mlp_train` で扱う。)

## 課題

`mnist_infer.cpp` (または `mnist_infer.f90`) は, 重みと画像を読み込み, 各画像を forward して予測クラスを求め, 正解率を表示する。各画像の推論は互いに**独立**なので並列化できる。

`TODO` の箇所に **OpenMP の指示を追加**し, 画像のループを並列化せよ。

- C++: ループ直前に `#pragma omp parallel for reduction(+:correct)` を加える (正解数を数えるので reduction)。
- Fortran: ループを `!$omp parallel do private(...) reduction(+:correct)` と `!$omp end parallel do` で囲む。

各画像の推論で使う一時配列 (隠れ層 `h`) は各スレッドで別々に持つ必要がある (C++ はループ内で宣言済み, Fortran は `private` 指定)。

## コンパイルと実行

```
# C++
nvc++ -fast -mp=multicore mnist_infer.cpp -o mnist_infer.exe

# Fortran
nvfortran -fast -mp=multicore mnist_infer.f90 -o mnist_infer.exe
```

```
OMP_NUM_THREADS=4 ./mnist_infer.exe
```

## 期待される結果

```
MNIST テスト 1000 枚: 正解 979 枚, 正解率 = 97.90%
```

- 学習済みの重みを使うので, **本当に手書き数字を約 98% 当てる**。並列化した行列積がそのまま認識器になっている。
- 各画像の計算は独立かつ決定的なので, **スレッド数を変えても正解率は完全に同じ** (97.90%) になる。
- スレッド数を増やすと速くなる (「性能を比べる」セルで台数効果を測れる)。
- (発展) GPU にオフロードする, 行列積を SIMD 化する, などにも挑戦してみよ。
