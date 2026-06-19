# 練習問題: GPUでベクトル加算 (map(to:) と map(from:))

## 目標

GPUで配列計算をするとき, **どのデータをGPUへ送り (to), どの結果をGPUから戻すか (from)** を `map` 節で正しく指定できるようになる.

## 課題

`vecadd.cpp` (または `vecadd.f90`) は, 配列 `a`, `b` を初期化し, `c[i] = a[i] + b[i]` を計算して検算する.
計算本体 (オフロードするループ) が抜けているので, 現状では `c` が初期値 `-1` のままで検算に失敗する.

`TODO` の箇所に **ループをGPUにオフロードして `c[i] = a[i] + b[i]` を計算する処理** を書け. その際, 入力 `a`, `b` はGPUへ送り, 結果 `c` はGPUから受け取るよう `map` 節を指定する.

- C++: `#pragma omp target teams distribute parallel for map(to: a[0:n], b[0:n]) map(from: c[0:n])` とその直後の `for` ループ.
- Fortran: `!$omp target teams distribute parallel do map(to: a, b) map(from: c)` … `do` ループ … `!$omp end target teams distribute parallel do`.

考えどころ: `a`, `b` は入力なので送るだけ (`to`), `c` は結果なので戻すだけ (`from`). `map(tofrom:)` でも動くが, 余計な転送を避けるには `to`/`from` を使い分けるのがよい.

## コンパイルと実行

```
# C++
nvc++ -mp=gpu vecadd.cpp -o vecadd.exe

# Fortran
nvfortran -mp=gpu vecadd.f90 -o vecadd.exe
```

GPUは計算ノードにのみ搭載されているので, `%%bash_submit` でジョブとして投入する.

```
#PJM -L rscgrp=lecture-a
#PJM -L elapse=0:10:00

./vecadd.exe 1000
```

## 期待される結果

正しくオフロード・転送できていれば検算に通り `OK` が表示される.

```
OK: c[0] = 0, c[999] = 2997
```

- オフロードを書く前は `c` が `-1` のままで `NG` になる.
- `map(from: c)` を忘れると, GPU上では計算できていてもホスト側の `c` が更新されず `NG` になることも確かめてみよ.
