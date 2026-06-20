# 応用問題 (集大成): 多層パーセプトロン (MLP) を自分で学習させる

## 目標

AI の「学習」の中身が実は **行列積の繰り返し** であることを, **多層パーセプトロン (MLP)** を一から実装して体験する。**forward (順伝播) → 損失 → backprop (逆伝播) → SGD (更新)** のループを自分で書き, **非線形な分類境界**を学習させる。

前の問題 (ロジスティック回帰) は線形分離可能なデータだった。今回は **線形では分けられない** データ (円の内外) を扱うので, **隠れ層** が必須である。

並列化対象は AI 学習の心臓部, **全サンプルにわたる勾配の和** である。

## 課題

合成データ (非線形分離):

- 各サンプルの座標を `[-1,1]^2` から乱数で生成 (`draw_rand01`)。
- ラベル `y_i = (x0^2 + x1^2 < R^2) ? 1 : 0` (**内側の円が class 1**)。直線では分けられない。

ネットワーク (入力 2 → 隠れ層 H → 出力 1):

- forward: `h_k = tanh(Σ_d W1[k,d] x_d + b1[k])`,  `o = sigmoid(Σ_k W2[k] h_k + b2)`
- 損失: 二値クロスエントロピー
- backprop: `do = o - y`,  `dW2[k] += do·h_k`,  `dh_k = do·W2[k]·(1 - h_k^2)`,  `dW1[k,d] += dh_k·x_d`, ...
- 更新 (フルバッチ勾配降下): 全サンプルにわたって勾配を**総和**し, `W -= lr·(勾配/N)`。

各エポックで一番重いのは **全 `N` サンプルにわたる forward + backprop の O(N·H)**。各サンプルの寄与は独立なので並列化できる。ただし勾配配列 `gW1, gb1, gW2, gb2` への加算は競合するので, **配列 reduction** で安全に総和する:

- C++: `#pragma omp parallel for reduction(+:loss,correct,gb2,gW1[:H*D],gb1[:H],gW2[:H])`
- Fortran: `!$omp parallel do private(...) reduction(+:loss,correct,gb2,gW1,gb1,gW2)` … `!$omp end parallel do` (サンプルごとの一時配列 `hh` は `private`)

これが `TODO` の並列化箇所である。スレッド数を変えても結果 (正解率) は同じになることを確認せよ。

## コンパイルと実行

```
# C++
nvc++ -fast -mp=multicore mlp_train.cpp -o mlp_train.exe

# Fortran
nvfortran -fast -mp=multicore mlp_train.f90 -o mlp_train.exe
```

引数: サンプル数 `N` (既定 4000), 隠れ層ユニット数 `H` (既定 32), エポック数 `E` (既定 3000), 学習率 `lr` (既定 0.7)。

```
OMP_NUM_THREADS=4 ./mlp_train.exe 4000 32 3000 0.7
```

## 期待される結果

```
epoch    0: loss=0.6951, acc=49.67%
epoch  500: loss=0.1388, acc=96.67%
...
epoch 2999: loss=0.0420, acc=99.38%
最終: N=4000, H=32, epochs=3000, loss=0.0420, acc=99.38%
elapsed = ... sec
```

- 学習が進むと **損失が下がり, 正解率が上がる**。隠れ層のおかげで非線形な円の境界を学習し, **最終的に 95% を大きく超える** (ほぼ 99%)。
- `OMP_NUM_THREADS` を増やすと `elapsed` が短くなる (台数効果)。正解率は本質的に同じになる。
- (発展) 内側の行列積を SIMD 化, あるいは GPU にオフロードして更に高速化できる。
