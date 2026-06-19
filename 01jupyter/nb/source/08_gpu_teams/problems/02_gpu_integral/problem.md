# 練習問題: GPUで数値積分 (reduction)

## 目標

`target teams distribute parallel for` と `reduction(+:s)` を使って, 意味のある計算 (数値積分) を GPU 上で実行できるようになる. 総和に使うスカラ変数は `map` を書かなくても自動的に転送されることを確認する.

## 課題

中点則 (長方形近似) で次の積分を計算すると `π` になる.

```
∫_0^1 4/(1+x^2) dx = π
```

区間 `[0,1]` を `n` 等分し, 各小区間の中点 `x = (i + 0.5)/n` での値 `4/(1+x^2)` を足し合わせ, 幅 `dx = 1/n` を掛ける.

`gpu_integral.cpp` (または `gpu_integral.f90`) の総和ループが抜けている. `TODO` の箇所に, **GPU にオフロードして `reduction(+:s)` で総和 `s` を求めるループ** を書け.

- C++: `#pragma omp target teams distribute parallel for reduction(+:s)` とその直後の `for` ループ.
- Fortran: `!$omp target teams distribute parallel do reduction(+:s) private(x)` … `do` ループ … `!$omp end target teams distribute parallel do`.

考えどころ: `s` は総和を入れるスカラなので, 配列の `map` のような指定は不要 (スカラはコンパイラが自動で扱う). このため, データ移動 (`map`) を学ぶ前でもこの問題は解ける.

## コンパイルと実行

```
# C++
nvc++ -mp=gpu gpu_integral.cpp -o gpu_integral.exe

# Fortran
nvfortran -mp=gpu gpu_integral.f90 -o gpu_integral.exe
```

GPU は計算ノードにのみ搭載されているので `%%bash_submit` でジョブとして投入する.

```
#PJM -L rscgrp=lecture-a
#PJM -L elapse=0:10:00

./gpu_integral.exe 100000000
```

## 期待される結果

`n` が十分大きければ `pi` が `π ≈ 3.14159265...` に近づく.

```
pi  = 3.141592653...
error = ...e-...
```

- `OMP_NUM_TEAMS` を変えて (例: 32, 256, 1024) 結果が変わらないこと, 実行時間が変わることを確かめてみよ.
- `n` を大きくすると誤差が小さくなることも確認せよ.
