# 練習問題: 配列を手作業でスレッドに分割する

## 目標

work-sharing 構文 (`for` / `do`) はまだ学んでいない. ここでは `omp_get_thread_num()` と `omp_get_num_threads()` だけを使って, 配列を**手作業で**スレッドに分割し, 各スレッドが自分の担当範囲の部分和を計算・表示する. 次のトピックで学ぶ `for` 構文が, なぜ便利なのかを先取りして体感する.

## 課題

`manual_partition.cpp` (または `manual_partition.f90`) は, 要素数 `n = 100` の配列 `a` (`a[i] = i + 1`) を用意し, スレッド `t` (全 `T` スレッド) が範囲 `[t*n/T, (t+1)*n/T)` を担当して, その部分和を表示する.

コメント `TODO` の指示に従って **OpenMP の指示行を追加** し, このブロックを複数スレッドで実行させよ.

- C++: ブロック `{ ... }` の直前に `#pragma omp parallel` を1行加える. `tid`, `nt`, `lo`, `hi`, `s` はブロック内で宣言してあるので, スレッドごとに別々の変数になる.
- Fortran: ブロックを `!$omp parallel private(tid, nt, lo, hi, i, s)` と `!$omp end parallel` で囲む. これらの変数をスレッドごとに別々にするため `private` 節を付ける.

**注意:** 1つの共有変数に全スレッドの和を足し込んではならない (それは競合 (data race) になる. 総和をまとめる `reduction` は後のトピック). 各スレッドは**自分の**部分和だけを表示すること.

それ以外のコードを変更する必要はない.

## コンパイルと実行

```
# C++
nvc++ -fast -mp=multicore manual_partition.cpp -o manual_partition.exe

# Fortran
nvfortran -fast -mp=multicore manual_partition.f90 -o manual_partition.exe
```

```
OMP_NUM_THREADS=4 ./manual_partition.exe
```

## 期待される結果

`OMP_NUM_THREADS` に指定した数だけ行が表示され, 各スレッドが重ならない範囲を担当する (順番は実行ごとに入れ替わってよい). 例 (4スレッド):

```
thread 0 of 4: range [0, 25), partial sum = 325.000000
thread 1 of 4: range [25, 50), partial sum = 950.000000
thread 2 of 4: range [50, 75), partial sum = 1575.000000
thread 3 of 4: range [75, 100), partial sum = 2200.000000
```

`OMP_NUM_THREADS` の値を変えると, 範囲の分割と表示行数が変わることを確認せよ. すべての部分和を足すと全体の和 (5050) になる.
