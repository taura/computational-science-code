# 練習問題: 並列化して台数効果を測る

## 目標

`#pragma omp parallel for` (Fortran では `!$omp parallel do` ... `!$omp end parallel do`) を1つ挿入するだけで, 独立な繰り返しからなる重いループを並列化し, スレッド数を増やすと性能 (GFLOPS) が上がる「台数効果」を測定できることを体験する.

## 課題

`measure_speedup.cpp` (または `measure_speedup.f90`) は, 各要素 `x[i]` を独立に重い計算で求め, 実行時間と GFLOPS を表示する.
現状は逐次 (並列化されていない) なので, スレッド数を増やしても速くならない.

コメント `TODO` の指示に従って **OpenMP の指示行を1つ追加** し, ループを並列化せよ.

- C++: 計算本体の `for` ループの直前に `#pragma omp parallel for` を1行加える.
- Fortran: 計算本体の `do` ループを `!$omp parallel do` と `!$omp end parallel do` で囲む.

それ以外のコードを変更する必要はない.

## コンパイルと実行

```
# C++
nvc++ -fast -mp=multicore measure_speedup.cpp -o measure_speedup.exe

# Fortran
nvfortran -fast -mp=multicore measure_speedup.f90 -o measure_speedup.exe
```

`OMP_NUM_THREADS` を 1, 2, 4, ... と変えながら実行し, GFLOPS を比較せよ. スレッドをコアに固定するため `OMP_PROC_BIND=true` も付ける.

```
OMP_PROC_BIND=true OMP_NUM_THREADS=1 ./measure_speedup.exe
OMP_PROC_BIND=true OMP_NUM_THREADS=2 ./measure_speedup.exe
OMP_PROC_BIND=true OMP_NUM_THREADS=4 ./measure_speedup.exe
```

第1引数で要素数 `m`, 第2引数で1要素あたりの反復数 `n` を変えられる (例: `./measure_speedup.exe 64 10000000`).

## 期待される結果

並列化後は, スレッド数を増やすと実行時間が短くなり, GFLOPS が増える (理想的にはスレッド数に比例). 例:

```
OMP_NUM_THREADS=1 ->  約  X GFLOPS
OMP_NUM_THREADS=2 -> 約 2X GFLOPS
OMP_NUM_THREADS=4 -> 約 4X GFLOPS
```

スレッド数を増やしても性能が頭打ちになる点も観察せよ.
