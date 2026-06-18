# 練習問題: schedule(runtime) で素数カウントの割り当て方を比べる

## 目標

繰り返しごとに**仕事量が偏る**ループ (素数の判定は大きい数ほど時間がかかる) を題材に, `schedule(runtime)` を使えば**再コンパイルなしに** `OMP_SCHEDULE` 環境変数だけで割り当て方 (static / dynamic / guided) を切り替えて性能を比較できることを体験する.

## 課題

`count_primes.cpp` (または `count_primes.f90`) は, `2` から `N` までの整数のうち素数の個数を試し割り (trial division) で数える. 大きい数ほど判定に時間がかかるため, 繰り返しの仕事量は一様ではない.

コメント `TODO` の指示に従ってループを並列化せよ. `schedule` には `runtime` を指定する.

- C++: ループの直前に `#pragma omp parallel for schedule(runtime) reduction(+:count)` を加える.
- Fortran: `do` ループを `!$omp parallel do schedule(runtime) reduction(+:count)` と `!$omp end parallel do` で囲む.

(`reduction(+:count)` は素数の個数の総和を競合なく集計するためのもの.)

## コンパイルと実行

```
# C++
nvc++ -fast -mp=multicore count_primes.cpp -o count_primes.exe

# Fortran
nvfortran -fast -mp=multicore count_primes.f90 -o count_primes.exe
```

`schedule(runtime)` なので, **コンパイルし直さず** `OMP_SCHEDULE` を変えるだけで割り当て方を切り替えられる. 実行時間を `time` で測って比べよ:

```
export OMP_NUM_THREADS=4

OMP_SCHEDULE=static          time ./count_primes.exe 300000
OMP_SCHEDULE=dynamic         time ./count_primes.exe 300000
OMP_SCHEDULE=dynamic,100     time ./count_primes.exe 300000
OMP_SCHEDULE=guided          time ./count_primes.exe 300000
```

## 期待される結果と考察

- 素数の個数 (`number of primes <= 300000 : 25997`) は, どの schedule でも**同じ**になることを確認せよ.
- 実行時間は schedule によって変わる. `static` は各スレッドに連番の塊を等分するため, 大きい数 (重い) を担当したスレッドだけ遅くなり, 全体が遅い側に引きずられる. `dynamic` や `guided` は終わったスレッドが次の塊を取りに行くので負荷の偏りが平準化され, 一般に速くなる.
- いくつかの schedule を試し, 最も速かったものを選べ. `dynamic,100` のようにチャンクサイズを変えると挙動が変わることも確認せよ.
