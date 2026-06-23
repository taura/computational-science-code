# 練習問題: マンデルブロ集合と schedule の選択

## 目標

仕事量が反復ごとに大きく異なるループを並列化し, `schedule` 節がなぜ重要かを体感する. 静的割り当て (`schedule(static)`) では負荷の不均衡で遅くなり, 動的割り当て (`schedule(dynamic)`) では負荷が均されて速くなることを `time` で確認する.

## 背景: マンデルブロ集合

複素数 `c` ごとに, `z = 0` から始めて `z ← z^2 + c` を繰り返す. `|z|^2 > 4` になったら「脱出」とみなし, そのときの反復回数を記録する. 集合内部の点は最後 (`maxiter`) まで脱出しないため反復回数が多く, 外側の点はすぐ脱出する. つまり**画素ごとに仕事量が大きく異なる**.

このプログラムは `W × H` の格子 (既定 2000×2000), 最大反復 `maxiter` (既定 2000) で各画素の脱出反復数を配列 `cnt` に格納し, 並列ループの後で総反復数を逐次に集計する (足し込みの競合を避けるため).

## 課題

`mandelbrot.cpp` (または `mandelbrot.f90`) の画素ループ (`px` ループ) を並列化せよ.

コメント `TODO` の指示に従って **OpenMP の指示を追加** する.

- C++: `px` ループの直前に `#pragma omp parallel for schedule(dynamic)` を1行加える.
- Fortran: `px` ループを `!$omp parallel do schedule(dynamic) private(...)` と `!$omp end parallel do` で囲む (`private` 節は雛形のコメントの通り).

画素ごとの仕事量が大きく異なるため, 各スレッドに均等な数の反復を割り当てる `schedule(static)` では, 集合内部を担当したスレッドだけ重くなり全体が遅くなる. 一方 `schedule(dynamic)` は終わったスレッドが次の仕事を取りに行くので負荷が均される.

総反復数 (`total iterations`) は **スケジュールによらず同じ値**になる (正しさの確認に使える).

## コンパイルと実行

```
# C++
nvc++ -fast -mp=multicore mandelbrot.cpp -o mandelbrot.exe

# Fortran
nvfortran -fast -mp=multicore mandelbrot.f90 -o mandelbrot.exe
```

```
OMP_NUM_THREADS=8 ./mandelbrot.exe
```

`schedule(dynamic)` で動かしたら, 指示を `schedule(static)` に書き換えて再コンパイルし, `time` で実行時間を比べよ:

```
OMP_NUM_THREADS=8 time ./mandelbrot.exe
```

## 期待される結果

```
W=2000 H=2000 maxiter=2000
total iterations = ...   (スケジュールによらず一定)
sample cnt: top-left=... center=2000 bottom-right=...
```

- `total iterations` は `schedule(dynamic)` でも `schedule(static)` でも **同じ**になる (正しさの確認).
- 中心 (center) の画素は集合内部なので反復数が `maxiter` (=2000) に達する.
- 実行時間は `schedule(dynamic)` の方が `schedule(static)` より短くなる (負荷分散の効果). スレッド数を増やすほど差が顕著になる.
