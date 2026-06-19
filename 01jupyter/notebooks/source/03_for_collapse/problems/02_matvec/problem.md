# 練習問題: 行列ベクトル積を for/do で並列化する

## 目標

work-sharing 構文 (`for` / `do`) を使って, 密な行列ベクトル積 `y = A x` を並列化する. 行ごとの計算を `#pragma omp parallel for` (Fortran は `!$omp parallel do`) でスレッドに分担させる. 各行の内積は局所変数に貯めるので `reduction` は不要であることを理解する.

## 背景

`n × n` の行列 `A` と長さ `n` のベクトル `x` の積 `y = A x` は

```
y[i] = Σ_j A[i][j] * x[j]   (i = 0, ..., n-1)
```

で定義される. 行列 `A` は1次元配列に `A[i*n + j]` として格納している. 各行 `i` の計算は他の行と独立なので, 外側の `i` ループを並列化できる.

## 課題

`matvec.cpp` (または `matvec.f90`) は, サイズ `n` (コマンドライン引数, 既定 4000) で `A[i*n+j] = 1`, `x[j] = 1` と初期化する. このとき正解は `y[i] = n` (全要素 `n`) になる.

コメント `TODO` の指示に従って **OpenMP の指示を追加** し, 行ループを並列化せよ.

- C++: 外側の `i` ループの直前に `#pragma omp parallel for` を1行加える. 内積の和 `s` はループ内で宣言してあるので, 反復ごと (= スレッドごと) に別々の変数になる.
- Fortran: 外側の `do i` ループを `!$omp parallel do private(j, s)` と `!$omp end parallel do` で囲む. 内側ループ変数 `j` と和 `s` をスレッドごとに別々にするため `private` 節に並べる.

並列化しても結果 (`y[i] = n`, 検算 `OK`) は変わらない. それ以外のコードを変更する必要はない.

**発展 (任意):** 初期化の二重ループ (`A[i*n+j] = 1`) は `collapse(2)` を付けて2重ループをまとめて並列化できる. 余裕があれば試してみよ. ただし採点の対象 (穴あき) は行列ベクトル積の `parallel for` の方である.

## コンパイルと実行

```
# C++
nvc++ -fast -mp=multicore matvec.cpp -o matvec.exe

# Fortran
nvfortran -fast -mp=multicore matvec.f90 -o matvec.exe
```

```
OMP_NUM_THREADS=4 ./matvec.exe
OMP_NUM_THREADS=8 ./matvec.exe 8000
```

## 期待される結果

```
n = 4000, y[0] = 4000.000000 (expected 4000): OK
```

`OK` が表示されれば正しく計算できている. スレッド数 (`OMP_NUM_THREADS`) を変えても結果は変わらず, `n` を大きくすると計算が重くなるので並列化の効果 (実行時間の短縮) を `time` などで確認できる.
