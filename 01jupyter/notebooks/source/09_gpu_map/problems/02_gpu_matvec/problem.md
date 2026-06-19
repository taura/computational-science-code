# 練習問題: GPUで行列ベクトル積 (行列の map)

## 目標

行列のような **2次元 (大きな) データ** を GPU へ送り, 結果のベクトルを受け取る `map` の使い方を身につける. ベクトルだけを扱った `01_vecadd` と違い, ここでは `n×n` の行列を転送する.

## 課題

行列ベクトル積 `y = A x` を GPU で計算する.

```
y[i] = Σ_j A[i][j] * x[j]
```

C++ では `A` を 1 次元配列とみなし `A[i*n+j]` でアクセスする (Fortran では `A(j,i)` の 2 次元配列).

`gpu_matvec.cpp` (または `gpu_matvec.f90`) の計算本体が抜けている. 初期状態では `y` が番兵 `-1` のままで検算に失敗する. `TODO` の箇所に, **行列ベクトル積を GPU にオフロードして計算する処理** を, 適切な `map` 節とともに書け.

- C++: `#pragma omp target teams distribute parallel for map(to: A[0:n*n], x[0:n]) map(from: y[0:n])` とその直後の二重ループ.
- Fortran: `!$omp target teams distribute parallel do map(to: A, x) map(from: y) private(j, s)` … ループ … `!$omp end target teams distribute parallel do`.

考えどころ:

- 入力 `A` (要素数 `n*n`) と `x` (要素数 `n`) は GPU へ送るだけ (`to`).
- 結果 `y` (要素数 `n`) は GPU から戻すだけ (`from`).
- C++ では配列セクションで転送量を明示する: `A[0:n*n]` は `n²` 要素, `x[0:n]`, `y[0:n]` は `n` 要素. 行列は `n²` に比例して転送量が大きいことを意識せよ.

## コンパイルと実行

```
# C++
nvc++ -mp=gpu gpu_matvec.cpp -o gpu_matvec.exe

# Fortran
nvfortran -mp=gpu gpu_matvec.f90 -o gpu_matvec.exe
```

GPU は計算ノードにのみ搭載されているので `%%bash_submit` でジョブとして投入する.

```
#PJM -L rscgrp=lecture-a
#PJM -L elapse=0:10:00

./gpu_matvec.exe 4096
```

## 期待される結果

`A` の全要素を `1`, `x` の全要素を `1` に初期化しているので, 正しく計算できれば `y[i] = n` となり `OK` が表示される.

```
OK: n = 4096, y[0] = 4096 (= n)
```

- `map(to: A ...)` を忘れると GPU 上の `A` が不定になり計算が壊れる.
- `map(from: y)` を忘れるとホスト側の `y` が更新されず番兵 `-1` のままで `NG` になる. 確かめてみよ.
