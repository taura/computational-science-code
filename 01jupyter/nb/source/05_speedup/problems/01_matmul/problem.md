# 練習問題: 行列積を並列化して台数効果を測る

## 目標

`#pragma omp parallel for` (Fortran では `!$omp parallel do` ... `!$omp end parallel do`) を1つ挿入するだけで,
HPC の代表的なカーネルである**密行列積** C = A * B を並列化し, スレッド数を増やすと性能 (GFLOPS) が上がる「台数効果」を測定できることを体験する.

## 課題

`matmul.cpp` (または `matmul.f90`) は, n x n の行列 A, B の積 C = A * B を 3 重ループで計算し, 実行時間と GFLOPS を表示する.
行列は 1 次元配列に格納している (A の (i,j) 要素が `A[i*n+j]`).
現状は逐次 (並列化されていない) なので, スレッド数を増やしても速くならない.

コメント `TODO` の指示に従って **OpenMP の指示行を1つ追加** し, **いちばん外側の i ループ** を並列化せよ.
各行 `C[i][*]` の計算は互いに独立で, k についての足し込みはループ内のローカル変数 `s` で行うので, reduction は不要である.

- C++: 計算本体の最も外側の `for` ループの直前に `#pragma omp parallel for` を1行加える.
- Fortran: 最も外側の `do` ループを `!$omp parallel do` と `!$omp end parallel do` で囲む.

それ以外のコードを変更する必要はない.

## コンパイルと実行

```
# C++
nvc++ -fast -mp=multicore matmul.cpp -o matmul.exe

# Fortran
nvfortran -fast -mp=multicore matmul.f90 -o matmul.exe
```

`OMP_NUM_THREADS` を 1, 2, 4, ... と変えながら実行し, GFLOPS を比較せよ. スレッドをコアに固定するため `OMP_PROC_BIND=true` も付ける.

```
OMP_PROC_BIND=true OMP_NUM_THREADS=1 ./matmul.exe
OMP_PROC_BIND=true OMP_NUM_THREADS=2 ./matmul.exe
OMP_PROC_BIND=true OMP_NUM_THREADS=4 ./matmul.exe
```

第1引数で行列のサイズ `n` を変えられる (例: `./matmul.exe 1500`).

## 期待される結果

A, B を全要素 1.0 で初期化しているので, 答えは全要素 `C[i][j] = n` となり, `checksum = n^3`, `OK` と表示される (並列化しても結果は変わらない).

並列化後は, スレッド数を増やすと実行時間が短くなり, GFLOPS が増える (理想的にはスレッド数に比例). 例:

```
OMP_NUM_THREADS=1 ->  約  X GFLOPS
OMP_NUM_THREADS=2 -> 約 2X GFLOPS
OMP_NUM_THREADS=4 -> 約 4X GFLOPS
```

これは人工的な計算ではなく, 科学技術計算で頻出する本物の行列積カーネルである.
スレッド数を増やしても性能が頭打ちになる点も観察せよ.
