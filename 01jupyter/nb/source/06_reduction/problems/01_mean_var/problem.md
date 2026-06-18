# 練習問題: 1つのループで平均と分散を求める (2変数の reduction)

## 目標

`reduction` 節に**複数の変数**を並べることで, 1つのループの中で2つの総和を同時に集計できることを体験する. ここでは配列の和 `s` と二乗和 `sq` を同時に求め, 平均と分散を計算する.

## 課題

`mean_var.cpp` (または `mean_var.f90`) は, 要素数 `n = 1000000` の配列 `a` (`a[i] = sin(i)`) について, 1つのループで

- 和    `s  += a[i]`
- 二乗和 `sq += a[i]*a[i]`

を求め, ループ後に `mean = s/n`, `variance = sq/n - mean*mean` を計算する.

コメント `TODO` の指示に従ってループを並列化せよ. **2つの総和を1つの `reduction` 節にまとめて**指定する.

- C++: ループの直前に `#pragma omp parallel for reduction(+:s,sq)` を加える.
- Fortran: `do` ループを `!$omp parallel do private(x) reduction(+:s,sq)` と `!$omp end parallel do` で囲む (一時変数 `x` はスレッドごとに別々にするため `private` にする).

`reduction(+:s,sq)` のように, カンマで区切って複数の変数を1つの節に並べられる.

## コンパイルと実行

```
# C++
nvc++ -fast -mp=multicore mean_var.cpp -o mean_var.exe

# Fortran
nvfortran -fast -mp=multicore mean_var.f90 -o mean_var.exe
```

```
OMP_NUM_THREADS=4 ./mean_var.exe
```

## 期待される結果

平均はほぼ 0, 分散はほぼ 0.5 になる (`sin` の値の性質による). 例:

```
mean = 0.000000, variance = 0.499999
```

`OMP_NUM_THREADS` を変えても結果はほぼ同じになる. ただし浮動小数点の和は**足す順序**によってごくわずかに変わるため, スレッド数を変えると最下位の桁が変動しうることに注意せよ.
