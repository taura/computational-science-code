# 練習問題: ループを複数スレッドで分担する

## 目標

`#pragma omp parallel for` (Fortran では `!$omp parallel do` ... `!$omp end parallel do`) を1つ挿入するだけで, ループの繰り返しを複数のスレッドに分担させられることを体験する.

## 課題

`parallel_for.cpp` (または `parallel_for.f90`) は, 配列 `a[i]` に独立な計算結果を書き込むループを, 現状では1スレッドで実行している.
各繰り返しがどのスレッドで実行されたかも表示する.

コメント `TODO` の指示に従って **OpenMP の指示行を1つ追加** し, ループの繰り返しが複数のスレッドに分担されるようにせよ.

- C++: `for` ループの直前に `#pragma omp parallel for` を1行加える.
- Fortran: `do` ループを `!$omp parallel do` と `!$omp end parallel do` で囲む.

それ以外のコードを変更する必要はない.

## コンパイルと実行

```
# C++
nvc++ -fast -mp=multicore parallel_for.cpp -o parallel_for.exe

# Fortran
nvfortran -fast -mp=multicore parallel_for.f90 -o parallel_for.exe
```

```
OMP_NUM_THREADS=4 ./parallel_for.exe
```

## 期待される結果

各繰り返し `i` が, いずれかのスレッドによって実行される. 1スレッドだけが全繰り返しを実行するのではなく, 複数のスレッド番号が現れる (表示の順番は実行ごとに入れ替わってよい). 例:

```
a[0] = 0  (thread 0)
a[1] = 1  (thread 0)
a[4] = 16 (thread 2)
a[2] = 4  (thread 1)
...
```

`OMP_NUM_THREADS` の値を変えて, 現れるスレッド番号が変わることを確認せよ.
