# 練習問題: 複数スレッドで挨拶する

## 目標

`#pragma omp parallel` (Fortran では `!$omp parallel`) を1つ挿入するだけで, 1つの `printf` (`print`) 文を複数のスレッドに実行させられることを体験する.

## 課題

`hello_threads.cpp` (または `hello_threads.f90`) は, 現状では1スレッドで1行だけ表示する.
コメント `TODO` の指示に従って **OpenMP の指示行を1つ追加** し, 挨拶がスレッドの数だけ表示されるようにせよ.

- C++: 挨拶を表示するブロック `{ ... }` の直前に `#pragma omp parallel` を1行加える.
- Fortran: `print` 文を `!$omp parallel` と `!$omp end parallel` で囲む.

それ以外のコードを変更する必要はない.

## コンパイルと実行

```
# C++
nvc++ -fast -mp=multicore hello_threads.cpp -o hello_threads.exe

# Fortran
nvfortran -fast -mp=multicore hello_threads.f90 -o hello_threads.exe
```

```
OMP_NUM_THREADS=4 ./hello_threads.exe
```

## 期待される結果

`OMP_NUM_THREADS` に指定した数だけ挨拶が表示される (順番は実行ごとに入れ替わってよい). 例:

```
hello from thread 0 of 4
hello from thread 2 of 4
hello from thread 1 of 4
hello from thread 3 of 4
```

`OMP_NUM_THREADS` の値を変えて, 表示される行数が変わることを確認せよ.
