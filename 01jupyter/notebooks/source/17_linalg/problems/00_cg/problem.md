# 練習問題: 共役勾配法 (CG) で連立一次方程式を解く

## 目標

対称正定値 (SPD) の大きな連立一次方程式 `A x = b` を, 反復解法の定番 **共役勾配法 (CG)** で解く。CG の中身は **行列ベクトル積 (matvec)** と **内積 (reduction)** とベクトルの足し算 (axpy) の繰り返しで, どれもこれまで学んだ並列化がそのまま使える。

ここで使う `A` は **B1 (2D熱伝導) と同じ 2次元ラプラシアン行列**。同じ行列を, B1 では「定常状態を反復で求めた」のに対し, ここでは「方程式として解く」。(さらに G2 では同じ行列の固有振動モードを求める。)

## 課題

`cg.cpp` (または `cg.f90`) は, ラプラシアン行列 `A` (行列を保持せず 5点ステンシルで計算する「行列フリー」) に対し, 既知の真の解 `xt` から `b = A xt` を作り, `x=0` から CG で `A x = b` を解く。

並列化すべき2つのカーネルが `TODO` になっている:

1. **matvec** (`y = A p`): 格子点の二重ループ。
   - C++: `#pragma omp parallel for collapse(2)`
   - Fortran: `!$omp parallel do collapse(2) private(v)` … `!$omp end parallel do`
2. **dot** (内積 `a・b`): 総和。
   - C++: `#pragma omp parallel for reduction(+:s)`
   - Fortran: `!$omp parallel do reduction(+:s)` … `!$omp end parallel do`

(CG 本体のベクトル更新 axpy は逐次のまま書いてある。余力があればそこも並列化してよい。)

## コンパイルと実行

```
# C++
nvc++ -fast -mp=multicore cg.cpp -o cg.exe

# Fortran
nvfortran -fast -mp=multicore cg.f90 -o cg.exe
```

第1引数で格子の一辺 `n` (未知数は `N = n²`), 第2引数で収束しきい値。

```
OMP_NUM_THREADS=4 ./cg.exe 128 1e-8
```

## 期待される結果

```
n=128 (N=16384), iters=400, 残差=9.94e-09, 解の誤差(max|x-xt|)=3.22e-09
```

- **残差** `||A x − b||` が `tol` 未満になれば収束。
- **解の誤差** `max|x − xt|` が小さければ, 真の解を正しく復元できている (検算成功)。
- CG の反復回数はおおよそ `n` に比例する (この問題では数百回)。
- `n` を大きくする (例: `512`) と1反復が重くなり, matvec と内積の並列化の効果 (台数効果) が見えやすい。「性能を比べる」セルで測ってみよ。
- (発展) 同じ matvec を GPU にオフロードする, axpy も並列化する, などで更に速くできる。
