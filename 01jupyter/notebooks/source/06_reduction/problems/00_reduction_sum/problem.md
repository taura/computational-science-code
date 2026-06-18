# 練習問題: reduction で総和の競合を解消する

## 目標

`reduction(+:s)` 句を1つ追加するだけで, 複数のスレッドが同じ変数 `s` に同時に加算しようとして起こる競合 (data race) を解消し, 正しい総和が得られることを体験する.

## 課題

`reduction_sum.cpp` (または `reduction_sum.f90`) は, 区間 [a, b] で `1/(1+x*x)` を数値積分するコードである.
このループを単純に並列化すると, 全スレッドが共有変数 `s` に同時に `s += ...` を行うため競合 (data race) が起き, スレッド数が2以上だと結果が間違ったり実行ごとに変わったりする.

コメント `TODO` の箇所で **`reduction` を用いて並列化せよ**. 各スレッドが部分和を持ち寄って正しい総和を得るようになる.

- C++: `#pragma omp parallel for reduction(+:s)`
- Fortran: `!$omp parallel do private(x) reduction(+:s)` … `!$omp end parallel do`

それ以外のコードを変更する必要はない.

## コンパイルと実行

```
# C++
nvc++ -fast -mp=multicore reduction_sum.cpp -o reduction_sum.exe

# Fortran
nvfortran -fast -mp=multicore reduction_sum.f90 -o reduction_sum.exe
```

```
OMP_NUM_THREADS=4 ./reduction_sum.exe
```

## 期待される結果

`[a, b] = [0, 1]` での `1/(1+x*x)` の積分は `π/4 ≒ 0.785398` である.

- `reduction(+:s)` を入れる前 (`OMP_NUM_THREADS=4`) は, 結果が 0.785398 から大きくずれ, 実行のたびに値が変わることがある (競合).
- `reduction(+:s)` を追加すると, スレッド数によらず常に `s = 0.785398` 付近になる.

```
s = 0.785398
```
