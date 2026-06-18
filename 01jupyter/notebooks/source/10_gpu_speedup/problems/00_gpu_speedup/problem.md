# 練習問題: GPU上での台数効果を測定する

## 目標

`#pragma omp target teams distribute parallel for` (Fortran では `!$omp target teams distribute parallel do` ... `!$omp end ...`) を1つ挿入して計算をGPUにオフロードし, チーム数・スレッド数を増やすとどれだけ速くなるか(台数効果)を測定する.

## 課題

`gpu_speedup.cpp` (または `gpu_speedup.f90`) は, `lin_rec` (`x = a*x + b` を `n` 回繰り返す関数) を `m` 個の独立な要素について計算するプログラムである.
現状ではオフロードの指示行が外してあり, 計算本体がホスト(CPU)上で逐次に実行されてしまう.

コメント `TODO` の指示に従って **オフロードの指示行を1つ追加** し, ループをGPU上で並列実行させよ.

- C++: `for` 文の直前に
  `#pragma omp target teams distribute parallel for num_teams(nteams) num_threads(nthreads) map(tofrom: x[0:m])`
  を1行加える.
- Fortran: `do` ループを
  `!$omp target teams distribute parallel do num_teams(nteams) num_threads(nthreads) map(tofrom: x)`
  と `!$omp end target teams distribute parallel do` で囲む.

結果を配列 `x` でCPUに戻して検算するので, `map(tofrom: x...)` が必要であることに注意.
それ以外のコードを変更する必要はない.

## コンパイルと実行

```
# C++
nvc++ -fast -mp=gpu gpu_speedup.cpp -o gpu_speedup.exe

# Fortran
nvfortran -fast -mp=gpu gpu_speedup.f90 -o gpu_speedup.exe
```

実行は

```
OMP_NUM_TEAMS=nteams OMP_NUM_THREADS=nthreads ./gpu_speedup.exe m n
```

とすると, チーム数=`nteams`, スレッド数=`nthreads` で実行する.
`m`, `n` を省略すると `m` = `nteams` × `nthreads`, `n` = 100×1000×1000 となる.

GPUは計算ノードにのみ搭載されているので, `%%bash_submit` でジョブとして投入する.
`OMP_NUM_THREADS` は 1 または 32 の倍数でなければならないことに注意 (GPUのスレッドは32本単位(ワープ)で動くため).

```
#PJM -L rscgrp=lecture-a
#PJM -L elapse=0:10:00

OMP_NUM_TEAMS=1 OMP_NUM_THREADS=1 ./gpu_speedup.exe
```

スレッド数を変えて一気に測るには:

```
#PJM -L rscgrp=lecture-a
#PJM -L elapse=0:10:00

for th in 1 32 64 128 256 512 1024 ; do
    echo -n "$th "
    OMP_NUM_TEAMS=1 OMP_NUM_THREADS=${th} ./gpu_speedup.exe | grep GFLOPS
done
```

チーム数を変えて測るには (スレッド数は上で良かった値に固定):

```
#PJM -L rscgrp=lecture-a
#PJM -L elapse=0:10:00

th=最適なスレッド数
for tm in 1 2 4 8 16 32 64 108 ; do
    echo -n "$((tm * th)) "
    OMP_NUM_TEAMS=${tm} OMP_NUM_THREADS=${th} ./gpu_speedup.exe | grep GFLOPS
done
```

## 期待される結果

正しくオフロードできていれば検算で `OK` が表示され, `GFLOPS` の値が出力される.

- `OMP_NUM_TEAMS=1` に固定し `OMP_NUM_THREADS` を 1, 32, 64, ... と増やすと, ある程度まではGFLOPSが向上する.
- 次にスレッド数を固定して `OMP_NUM_TEAMS` を増やすと, このGPU (NVIDIA A100, 108個のStreaming Multiprocessor) ではおおむね108程度までGFLOPSが向上し, それ以降は頭打ちになる.

チーム数・スレッド数が少なすぎるとGPUの演算器を使い切れず性能が出ないこと, 両方を十分大きくして初めてGPUの性能を引き出せることを確認せよ.
