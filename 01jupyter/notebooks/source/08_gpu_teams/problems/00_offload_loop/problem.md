# 練習問題: ループをGPUで並列実行する

## 目標

`#pragma omp target teams distribute parallel for` (Fortran では `!$omp target teams distribute parallel do` ... `!$omp end target teams distribute parallel do`) を1つ挿入するだけで, 1重ループをGPU上の多数のチーム×スレッドに分割して並列実行できることを体験する.

## 課題

`offload_loop.cpp` (または `offload_loop.f90`) は, 現状では `m` 回まわるループを逐次に実行し, 各繰り返しが「自分の繰り返し番号・チーム番号・スレッド番号」を表示する.
コメント `TODO` の指示に従って **OpenMP の指示行を1つ追加** し, このループをGPU上で並列に実行させよ.

- C++: `for` 文の直前に `#pragma omp target teams distribute parallel for` を1行加える.
- Fortran: `do` ループを `!$omp target teams distribute parallel do` と `!$omp end target teams distribute parallel do` で囲む.

ループは結果を配列に書き戻さず表示するだけなので, `map` 節を考える必要はない.
それ以外のコードを変更する必要はない.

## コンパイルと実行

```
# C++
nvc++ -mp=gpu offload_loop.cpp -o offload_loop.exe

# Fortran
nvfortran -mp=gpu offload_loop.f90 -o offload_loop.exe
```

GPUは計算ノードにのみ搭載されているので, `%%bash_submit` でジョブとして投入して実行する.
チーム数は `OMP_NUM_TEAMS`, 1チームあたりのスレッド数は `OMP_NUM_THREADS` で指定する.
`OMP_NUM_THREADS` は 1 または 32 の倍数でなければならないことに注意 (GPUのスレッドは32本単位(ワープ)で動くため).

```
#PJM -L rscgrp=lecture-a
#PJM -L elapse=0:10:00

OMP_NUM_TEAMS=2 OMP_NUM_THREADS=32 ./offload_loop.exe 8
```

## 期待される結果

並列化前(逐次)では, すべての繰り返しがチーム0・スレッド0で実行され, 表示も繰り返し番号順に並ぶ.

並列化後は, 各繰り返しが複数のチーム・スレッドに分散して実行される. 表示の順番は実行ごとに入れ替わってよい. 例 (`m=8`):

```
i = 3  executed by team 1  thread 3
i = 0  executed by team 0  thread 0
i = 5  executed by team 1  thread 5
...
```

`OMP_NUM_TEAMS` や `OMP_NUM_THREADS`, コマンドライン引数 `m` を変えて, 繰り返しがどのチーム・スレッドに割り当てられるか観察せよ.
