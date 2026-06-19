# 練習問題: GPUで行列積の性能測定 (CPU との比較)

## 目標

意味のある計算 (行列積 `C = A * B`) を GPU で実行し, **GFLOPS で性能を測定**して CPU (ホスト) 実行と比較する. 同じソース・同じディレクティブが, 環境変数だけで CPU でも GPU でも走ることを体感する.

## 課題

`gpu_matmul.cpp` (または `gpu_matmul.f90`) は行列積を計算する完成済みプログラムである (オフロード指示は既に書かれている). 浮動小数点演算数は `2 n³` で, `omp_get_wtime` で経過時間を測り GFLOPS を表示する.

このプログラムを **GPU 実行**と **CPU (ホスト) 実行**の両方で, いくつかの `n` について動かし, GFLOPS を比較せよ.

- GPU 実行: `OMP_TARGET_OFFLOAD=MANDATORY` (オフロードを強制. 未指定でも `-mp=gpu` でビルドすれば GPU で走る).
- CPU 実行: `OMP_TARGET_OFFLOAD=DISABLED` とし, `OMP_NUM_THREADS` でスレッド数を指定 (1 または 32 の倍数).

## コンパイルと実行

```
# C++
nvc++ -fast -mp=gpu gpu_matmul.cpp -o gpu_matmul.exe

# Fortran
nvfortran -fast -mp=gpu gpu_matmul.f90 -o gpu_matmul.exe
```

GPU は計算ノードにのみ搭載されているので `%%bash_submit` でジョブとして投入する.

```
#PJM -L rscgrp=lecture-a
#PJM -L elapse=0:10:00

echo "=== GPU ==="
for n in 512 1024 2048; do
  OMP_TARGET_OFFLOAD=MANDATORY ./gpu_matmul.exe $n
done

echo "=== CPU (32 threads) ==="
for n in 512 1024 2048; do
  OMP_TARGET_OFFLOAD=DISABLED OMP_NUM_THREADS=32 ./gpu_matmul.exe $n
done
```

## 期待される結果

各行に `n`, 経過時間, GFLOPS, 検算 (`OK`) が表示される.

```
=== GPU ===
n = 512, elapsed = ..., ... GFLOPS, OK
...
```

考えどころ:

- `n` が小さいうちは, GPU はデータ転送・カーネル起動のオーバーヘッドが効いて CPU に対する優位が小さい. `n` を大きくすると GPU の GFLOPS が伸びてくる.
- `05_speedup/01_matmul` の CPU 行列積 (スレッド数を増やしての台数効果) と, ここでの GPU の GFLOPS を比べてみよ. GPU の方がどの程度速いか, どの `n` から有利になるかを考察せよ.
