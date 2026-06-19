# 練習問題: CPU と GPU はどちらが速い? (損益分岐点を探す)

## 目標

GPU は「十分に大きな並列計算」では CPU より速いが, 仕事が小さいとデータ転送や起動の overhead に負けて **CPU の方が速い** こともある. 同じプログラムを CPU と GPU で動かし, GPU が勝ち始める問題サイズ (損益分岐点) を体験する.

## 課題

`cpu_vs_gpu.cpp` (または `cpu_vs_gpu.f90`) は, `lin_rec` (`x = a*x + b` を `n` 回) を `m` 個の独立な要素について計算し, GFLOPS を表示する.
このループには **すでにGPU用のオフロード指示が付いている** (穴埋めは不要).

同じ実行ファイルを, 環境変数 `OMP_TARGET_OFFLOAD` で実行先を切り替えて比較する:

- `OMP_TARGET_OFFLOAD=DISABLED` … ホスト (CPU) で実行
- `OMP_TARGET_OFFLOAD=MANDATORY` … デバイス (GPU) で実行 (GPUが無ければエラー)

問題サイズ `m` を小さい値から大きい値まで変えながら両方を測り, **どのあたりの `m` で GPU が CPU を逆転するか** を調べよ. (CPU実行時は `OMP_NUM_THREADS` でスレッド数も指定できる.)

## コンパイルと実行

```
# C++
nvc++ -fast -mp=gpu cpu_vs_gpu.cpp -o cpu_vs_gpu.exe

# Fortran
nvfortran -fast -mp=gpu cpu_vs_gpu.f90 -o cpu_vs_gpu.exe
```

GPUは計算ノードにのみ搭載されているので, `%%bash_submit` でジョブとして投入する.

```
#PJM -L rscgrp=lecture-a
#PJM -L elapse=0:10:00

for m in 1 8 64 512 4096 32768 262144 ; do
    echo -n "CPU m=$m : "
    OMP_TARGET_OFFLOAD=DISABLED OMP_NUM_THREADS=9 ./cpu_vs_gpu.exe $m | grep GFLOPS
    echo -n "GPU m=$m : "
    OMP_TARGET_OFFLOAD=MANDATORY ./cpu_vs_gpu.exe $m | grep GFLOPS
done
```

## 期待される結果

- `m` が小さいうちは CPU の方が GFLOPS が高い (GPU は並列度を使い切れず, overhead が目立つ).
- `m` を大きくしていくと, あるサイズを境に GPU が CPU を上回り, さらに大きくすると差が開く.

「GPU は何でも速い」わけではなく, **十分な仕事量があって初めて GPU が活きる** ことを, 損益分岐点の `m` の値とともに確認せよ.
