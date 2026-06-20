# 応用問題 (最上級・発展): 物理情報ニューラルネット (PINN) で微分方程式を解く

## 目標

微分方程式

```
-u''(x) = f(x),   u(0) = u(1) = 0
```

を, 差分法 (B系) でも CG法 (G1) でもなく, **ニューラルネット (機械学習)** で解く。これが **物理情報ニューラルネット (PINN, Physics-Informed Neural Network)** の考え方である。**同じ問題を全く別の道具で解く**ことを体験する, 最上級の発展課題である。

PINN では, NN の出力 `u(x)` を解の候補とし, **PDE の残差そのものを損失**にして「解になるように」パラメータを学習する。

## 課題

1隠れ層・`tanh` のネットを使うと, `u` の微分は **解析的に書ける** (自動微分エンジン不要):

```
u(x)   = Σ_k a_k tanh(z_k),         z_k = w_k x + b_k
u'(x)  = Σ_k a_k w_k   sech^2(z_k),  sech^2 = 1 - tanh^2
u''(x) = Σ_k a_k w_k^2 (-2 tanh(z_k) sech^2(z_k))
```

製造解 (manufactured solution) として `u*(x) = sin(pi x)` (`u(0)=u(1)=0` を満たす) を選ぶと, `f(x) = -u*'' = pi^2 sin(pi x)`。

損失 (PDE 残差 + 境界ペナルティ):

```
loss = (1/M) Σ_i ( -u''(x_i) - f(x_i) )^2  +  lambda_bc ( u(0)^2 + u(1)^2 )
```

`M` 個のコロケーション点 `x_i ∈ (0,1)` で残差を測り, `lambda_bc ~ 10`。

パラメータ `{a_k, w_k, b_k}` (合計 `3H` 個) を勾配降下で学習する。勾配は**パラメータについての差分 (有限差分)** で求める:

```
grad_j = ( loss(p + eps·e_j) - loss(p - eps·e_j) ) / (2 eps)
```

この **`3H` 個の評価は互いに独立** (各 `j` で `loss` を 2 回呼ぶだけ) なので, **パラメータ番号 `j` で並列化**できる。`loss` はパラメータ配列の純関数 (各スレッドが自分のコピーを摂動) なのでスレッドセーフである。

- C++: `#pragma omp parallel for` を `for (j ...)` に付ける
- Fortran: `!$omp parallel do private(j,q,pp,lp,lm)` … `!$omp end parallel do`

これが `TODO` の並列化箇所である (パラメータについて embarrassingly parallel)。

## コンパイルと実行

```
# C++
nvc++ -fast -mp=multicore pinn.cpp -o pinn.exe

# Fortran
nvfortran -fast -mp=multicore pinn.f90 -o pinn.exe
```

引数: 隠れユニット数 `H` (既定 16), コロケーション点数 `M` (既定 50), ステップ数 `steps` (既定 4000), 学習率 `lr` (既定 0.01)。

```
OMP_NUM_THREADS=4 ./pinn.exe 16 50 4000 0.01
```

## 期待される結果

```
step    0: loss=4.252154e+01
step 1000: loss=1.129228e-02
step 2000: loss=9.033110e-04
step 3000: loss=4.090927e-04
step 3999: loss=3.807349e-04
H=16, M=50, steps=4000, lr=0.01
final max|u - sin(pi x)| = 1.2837e-04
elapsed = ... sec
```

- 学習が進むと **損失が下がり**, 学習した `u(x)` が厳密解 `sin(pi x)` に近づく。最終的な **最大誤差は 0.05 を大きく下回る** (約 1e-4)。
- `OMP_NUM_THREADS` を増やすと `elapsed` が短くなる (台数効果)。結果 (誤差) は本質的に同じになる。
- 同じ `-u''=f` を, B系 (差分法) や G1 (CG法) とは全く別の道具 (機械学習) で解いたことを味わってほしい。
