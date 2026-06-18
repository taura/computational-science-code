# 練習問題: 本当にGPUで実行されているか確かめる

## 目標

`#pragma omp target` (Fortran では `!$omp target` ... `!$omp end target`) は, GPUが使えないときには **黙ってCPUにフォールバック** して実行される.
そのため「GPUで動かしているつもりが, 実はCPUで動いていた」という事故が起こりうる.

そこで `omp_is_initial_device()` を使って, ある領域が実際にどこで実行されたかを判定する方法と, 環境変数 `OMP_TARGET_OFFLOAD` で実行場所を制御する方法を体験する.

- `omp_is_initial_device()` は, **ホスト(CPU)上では 1**, **デバイス(GPU)上では 0** を返す.

## 課題

`where_am_i.cpp` (または `where_am_i.f90`) は, まずホスト上で `omp_is_initial_device()` の値を表示し, 続いて `target` 領域の中で同じ関数の値を表示する.
コメント `TODO` の指示に従って **OpenMP の指示行を1つ追加** し, 後半の表示が `target` 領域(GPU上)で実行されるようにせよ.

- C++: メッセージを表示するブロック `{ ... }` の直前に `#pragma omp target` を1行加える.
- Fortran: `print` 文を `!$omp target` と `!$omp end target` で囲む.

表示するだけなので `map` 節を考える必要はない. それ以外のコードを変更する必要はない.

## コンパイルと実行

```
# C++
nvc++ -mp=gpu where_am_i.cpp -o where_am_i.exe

# Fortran
nvfortran -mp=gpu where_am_i.f90 -o where_am_i.exe
```

GPUは計算ノードにのみ搭載されているので, `%%bash_submit` でジョブとして投入して実行する.
以下の **3通り** の方法で実行し, `inside target:` の行に表示される値を比べよ.

```
#PJM -L rscgrp=lecture-a
#PJM -L elapse=0:10:00

# (1) 既定の動作 (GPUがあればGPU, なければCPU)
./where_am_i.exe

# (2) GPUでの実行を強制 (GPUが使えなければエラー)
OMP_TARGET_OFFLOAD=MANDATORY ./where_am_i.exe

# (3) 必ずCPUで実行
OMP_TARGET_OFFLOAD=DISABLED ./where_am_i.exe
```

## 期待される結果

- `inside target:` の値が **0** なら, その領域は **GPU** で実行されている.
- `inside target:` の値が **1** なら, その領域は **CPU** で実行されている (フォールバック).
- `on host:` の値は常に 1 (こちらは必ずホストで実行されるため).

計算ノードでの想定:

- (2) `MANDATORY` では `inside target:` は **0** になる (GPUが使われている).
- (3) `DISABLED` では `inside target:` は **1** になる (CPUに強制).
- (1) 既定では, 計算ノードにGPUがあれば **0** になるはず.

このように, `printf` / `print` の出力だけでは区別がつかない「どこで実行されたか」を, `omp_is_initial_device()` で確実に判定できる.
GPU向けのコードを書いたら, 思い込みで終わらせず実際にGPUで動いていることを確認する習慣をつけよう.
