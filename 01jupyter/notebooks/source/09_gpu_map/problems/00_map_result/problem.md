# 練習問題: GPUの計算結果をCPUに戻す

## 目標

`map(tofrom: ...)` (または `map(from: ...)`) 節を1つ加えるだけで, GPU上で書き換えた値がCPU側に戻ってくることを体験する.

## 課題

`map_result.cpp` (または `map_result.f90`) は, スカラ `t` と配列 `a` を用意し,
`#pragma omp target` (`!$omp target`) 領域の中でそれらをGPU上で2倍に書き換える.
しかし現状では `target` に `map` 節が付いていないため, GPUでの書き換えがCPUに戻ってこない.

コメント `TODO` の指示に従って **`target` 構文に適切な `map` 句を付けて結果をホストへ戻せ**.

- C++: `#pragma omp target` に `map(tofrom: t, a[0:3])` を加える.
- Fortran: `!$omp target` に `map(tofrom: t, a)` を加える (Fortranでは配列全体は単に `a` と書ける).

それ以外のコードを変更する必要はない.

## コンパイルと実行

```
# C++
nvc++ -mp=gpu map_result.cpp -o map_result.exe

# Fortran
nvfortran -mp=gpu map_result.f90 -o map_result.exe
```

GPUは計算ノードにのみ搭載されているので, `%%bash_submit` でジョブとして投入して実行する.

```
#PJM -L rscgrp=lecture-a
#PJM -L elapse=0:10:00

./map_result.exe
```

## 期待される結果

`map` 節を付ける前は, GPUでは2倍になっているが, CPU側の表示は元の値のままになる(書き換えが戻ってこない). 例:

```
GPU: t = 10.000000
GPU: a = { 11.000000, 12.000000, 13.000000 }
CPU: t = 10.000000
CPU: a = { 11.000000, 12.000000, 13.000000 }
```

`map(tofrom: ...)` を正しく加えた後は, CPU側の表示も2倍になる.

```
GPU: t = 10.000000
GPU: a = { 11.000000, 12.000000, 13.000000 }
CPU: t = 20.000000
CPU: a = { 22.000000, 24.000000, 26.000000 }
```

値を戻すだけなら `map(tofrom: ...)` の代わりに `map(from: ...)` でもよいことも確認してみよ.
