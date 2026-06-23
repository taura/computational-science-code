# 応用問題: 勾配降下法でロジスティック回帰を学習する

## 目標

データから重みを**学習**する最も基本的な機械学習である **ロジスティック回帰** を, **勾配降下法 (gradient descent)** で訓練する。学習が進むにつれて**正解率が上がっていく**様子を観察する。

並列化対象は AI 学習の心臓部, **全サンプルにわたる予測と勾配の和** (行列積 + reduction) である。

## 課題

合成データを使う:

- 真の重み `w_true[D]` を乱数で決め, 各サンプル `i` の特徴 `x_i[d] = draw_rand01(i,d) - 0.5` を生成。
- ラベル `y_i = (w_true・x_i > 0) ? 1 : 0` (**線形分離可能**)。

ロジスティック回帰:

- 予測 `p = sigmoid(w・x)`, 損失は二値クロスエントロピー。
- 勾配 `grad[d] = (1/N) Σ_i (sigmoid(w・x_i) - y_i) * x_i[d]`。
- `w` を 0 から始め, 各エポックで `w[d] -= lr * grad[d]`。

各エポックで一番重いのは **全 `N` サンプルにわたる O(N·D) の計算**。これを 2 つのループに分けてある:

1. **サンプルのループ** (誤差 `err[i] = p - y_i` の計算 + 損失・正解数の集計)。各サンプルは独立。
   - C++: `#pragma omp parallel for reduction(+:loss,correct)`
   - Fortran: `!$omp parallel do private(...) reduction(+:loss,correct)` … `!$omp end parallel do`

これが `TODO` の並列化箇所である。

2. 勾配 `grad[d]` のループは特徴 `d` で並列化済み (`#pragma omp parallel for` / `!$omp parallel do`, 競合なし)。サンプルにわたる和を `err[]` に分けたので reduction の競合を避けている。

## コンパイルと実行

```
# C++
nvc++ -fast -mp=multicore regression.cpp -o regression.exe

# Fortran
nvfortran -fast -mp=multicore regression.f90 -o regression.exe
```

引数: サンプル数 `N` (既定 200000), 特徴次元 `D` (既定 20), エポック数 `E` (既定 130), 学習率 `lr` (既定 1.0)。

```
OMP_NUM_THREADS=4 ./regression.exe 200000 20 130 1.0
```

## 期待される結果

```
epoch   0: loss=0.6931, acc=...%
epoch  50: loss=..., acc=...%
...
最終: N=200000, D=20, epochs=130, loss=..., acc=99..%
elapsed = ... sec
```

- 学習が進むと **損失が下がり, 正解率が上がる** (線形分離可能なデータなので最終的に **95% を大きく超える**, ほぼ 99%+)。
- `OMP_NUM_THREADS` を増やすと `elapsed` が短くなる (台数効果)。結果 (正解率) は本質的に同じになる。
- (発展) 内側の `w・x` 積を SIMD 化, あるいは GPU にオフロードして更に高速化できる。
