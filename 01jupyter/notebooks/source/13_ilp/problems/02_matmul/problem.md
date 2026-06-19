# 練習問題: 行列積をマルチコア + SIMD で高速化する (総仕上げ)

## 目標

行列積 `C = A * B` という現実的なカーネルを, **マルチコア並列 (parallel for) と SIMD (omp simd) の両方**で高速化する. これまで別々に学んだ2つの並列化を1つのカーネルで組み合わせる, この授業の総仕上げである.

ループは i-k-j 順とし, 外側 `i` ループは行ごとに独立なのでスレッド並列に, 最内 `j` ループは連続したメモリへの積和なのでSIMD化に向いている.

## 課題

`matmul.cpp` (または `matmul.f90`) では, 外側 `i` ループの `#pragma omp parallel for` (Fortran は `!$omp parallel do`) は **既に与えられている**.
コメント `TODO` の指示に従い, **最内 `j` ループをSIMD化する指示行を1つ追加**せよ.

- C++: 最内 `for (j ...)` の直前に `#pragma omp simd` を1行加える.
- Fortran: 最内 `do j` の直前に `!$omp simd` を1行加える.

これで「外: マルチコア並列」「内: SIMD」という二重の並列化になる.

## コンパイルと実行

`parallel for` と `omp simd` の両方を使うので `-mp=multicore` が必要.

```
# C++
nvc++ -fast -mp=multicore matmul.cpp -o matmul_cpp.exe
# Fortran
nvfortran -fast -mp=multicore matmul.f90 -o matmul_f90.exe

# スレッド数を変えて GFLOPS を測る
OMP_PROC_BIND=true OMP_NUM_THREADS=1  ./matmul_cpp.exe
OMP_PROC_BIND=true OMP_NUM_THREADS=8  ./matmul_cpp.exe
```

`n` はコマンドライン引数で指定できる (既定 1024). `A[i]=1`, `B[i]=2` としているので各要素は `2*n` になり, `check: OK` で正しさを確認できる.

## 期待される結果

- `n=1024 : XX.XXX GFLOPS  (check: OK)` のように表示される.
- `OMP_NUM_THREADS` を `1, 2, 4, 8, ...` と増やすと GFLOPS が伸びる (台数効果). `OMP_PROC_BIND=true` でスレッドをコアに固定すると安定する.
- 05_speedup の matmul はマルチコア化のみ, 本問はそこに SIMD を重ねたものである. GPU 版の行列積 (10 番台) とも比較し, CPU のピーク性能 (コア数 × SIMD幅 × FMA × クロック) にどこまで近づけるか考えてみよ.
