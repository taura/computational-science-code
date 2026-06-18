# 練習問題: 実行をGPUに移す

## 目標

`#pragma omp target` (Fortran では `!$omp target` ... `!$omp end target`) を1つ挿入するだけで, 1つの文の実行をデバイス(GPU)に移せることを体験する.

## 課題

`target_offload.cpp` (または `target_offload.f90`) は, 現状ではホスト(CPU)上で1行のメッセージを表示する.
コメント `TODO` の指示に従って **OpenMP の指示行を1つ追加** し, その `printf` (`print`) 文がGPU上で実行されるようにせよ.

- C++: メッセージを表示するブロック `{ ... }` の直前に `#pragma omp target` を1行加える.
- Fortran: `print` 文を `!$omp target` と `!$omp end target` で囲む.

それ以外のコードを変更する必要はない.

## コンパイルと実行

```
# C++
nvc++ -mp=gpu target_offload.cpp -o target_offload.exe

# Fortran
nvfortran -mp=gpu target_offload.f90 -o target_offload.exe
```

GPUは計算ノードにのみ搭載されているので, `%%bash_submit` でジョブとして投入して実行する.
必要に応じて rscgrp や elapse を指定せよ.

```
#PJM -L rscgrp=lecture-a
#PJM -L elapse=0:10:00

OMP_TARGET_OFFLOAD=MANDATORY ./target_offload.exe
```

## 期待される結果

メッセージが表示される.

```
hello from the device
```

`printf` / `print` の出力結果自体はCPUで実行してもGPUで実行しても同じなので, 見た目では区別がつかない. そこで `OMP_TARGET_OFFLOAD` を切り替えて挙動を比べてみよ.

- `OMP_TARGET_OFFLOAD=MANDATORY` ... GPUでの実行を強制する. `target` の挿入を忘れていてもエラーにはならない(`target` 領域がなければGPUを使わないため)が, GPUがないログインノードで `target` 付きで実行するとエラーになる.
- `OMP_TARGET_OFFLOAD=DISABLED` ... 必ずCPUで実行する.

`target` を正しく挿入したうえで, 計算ノードで `MANDATORY` を指定して(GPUで)実行できることを確認せよ.
