# 練習問題: SIMD + マルチコアで台数効果を出す

## 目標

SIMD (ベクトル型) によって1コア内で性能を出しているコードに `#pragma omp parallel for` (Fortran では `!$omp parallel do`) を1つ追加するだけで, 外側のループがマルチコア並列化され, スレッド数に応じてGFLOPS値が伸びることを体験する.

## 課題

`simd_multicore.cpp` (または `simd_multicore.f90`) は, 互いに独立な漸化式 `t = a*t + b` を多数 (`m` 本) 計算するプログラムである.

- C++版は, ベクトル型 `doublev` (`double` を `nl` 個束ねた型) を使い, `nl` 本の漸化式を1命令でまとめて進めることでSIMD化されている. しかし外側のループ (`i += nl`) はまだ1スレッドでしか動かない.
- Fortran版は, 内側の漸化式を `!$omp simd` でSIMD化してあるが, やはり外側のループは1スレッドである.

コメント `TODO` の指示に従って **OpenMP の指示行を1つ追加** し, 外側のループをマルチコア並列化せよ.

- C++: `for (long i = 0; i < m; i += nl)` の直前に `#pragma omp parallel for` を1行加える.
- Fortran: `do i = 1, m` の直前に `!$omp parallel do` を1行加える.

それ以外のコードを変更する必要はない.

なお C++版の先頭には `enum { nl = 8 };` というベクトル長の定義がある. 課題が動いたら, `nl` を 8, 16, 32 (2のべき乗) と変えて性能がどう変わるかも試してみよ.

<font color="red">注:</font> ベクトル型 (`vector_size`) はC/C++独自の拡張であり, Fortranには相当する機能が無い. そのためFortran版ではSIMD化を `!$omp simd` (内側) に, マルチコア並列化を `!$omp parallel do` (外側) に委ねている.

## コンパイルと実行

```
# C++
nvc++ -fast -mp=multicore simd_multicore.cpp -o simd_multicore.exe

# Fortran
nvfortran -fast -mp=multicore simd_multicore.f90 -o simd_multicore.exe
```

`OMP_NUM_THREADS` を変えながら実行する. `OMP_PROC_BIND=true` は各スレッドを特定のコアに固定し, 台数効果の測定を安定させる指示である. `m` はスレッド数に比例させて与える (各スレッドの仕事量を一定に保つ).

```
OMP_PROC_BIND=true OMP_NUM_THREADS=1 ./simd_multicore.exe 64 $((100 * 1000 * 1000))
```

ジョブとして投入する場合の例 (`#PJM` の指定):

```
#PJM -L rscgrp=lecture-a

for th in 1 2 4 8 ; do
    OMP_PROC_BIND=true OMP_NUM_THREADS=${th} ./simd_multicore.exe $((64 * ${th})) $((100 * 1000 * 1000)) | grep GFLOPS
done
```

## 期待される結果

`#pragma omp parallel for` (`!$omp parallel do`) を追加すると, `OMP_NUM_THREADS` を増やすにつれてGFLOPS値がほぼスレッド数に比例して伸びる.
追加する前は (1スレッドでしか動かないため) スレッド数を増やしてもGFLOPS値が変わらないことと比べてみよ.
最後に `OK` と表示されれば計算結果は正しい.
