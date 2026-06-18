# 練習問題: omp simd でループをSIMD化する

## 目標

`#pragma omp simd` (Fortran では `!$omp simd`) を1つ挿入するだけで, コンパイラがループをSIMD命令 (`pd` 付きの packed double 命令) に変換することを体験する.

## 課題

`omp_simd.cpp` (または `omp_simd.f90`) は, saxpy/axpy と呼ばれる典型的な演算 `y[i] = a * x[i] + y[i]` を行う関数である.
配列 `x`, `y` が重なっている (エイリアスしている) 可能性をコンパイラが排除できないと, 安全のため自動ベクトル化を諦めてしまうことがある.

コメント `TODO` の指示に従って **OpenMP の指示行を1つ追加** し, ループがSIMD化されるようにせよ.

- C++: ループの直前に `#pragma omp simd` を1行加える.
- Fortran: doループの直前に `!$omp simd` を1行加える.

それ以外のコードを変更する必要はない.

## コンパイルと実行

`#pragma omp simd` を解釈させるには `-mp=multicore` が必要である (SIMD命令を使うだけなので1スレッドで動作する).

```
# C++
nvc++ -fast -mp=multicore -Mkeepasm -Minfo -Mneginfo -c omp_simd.cpp

# Fortran
nvfortran -fast -mp=multicore -Mkeepasm -Minfo -Mneginfo -c omp_simd.f90
```

- `-Mkeepasm` : 生成されたアセンブリ言語のファイル (`omp_simd.s`) を残す
- `-Minfo` / `-Mneginfo` : SIMD化に成功した・失敗したことを報告してくれる

生成されたアセンブリを確認する.

```
cat omp_simd.s
```

## 期待される結果

`#pragma omp simd` (`!$omp simd`) を追加すると, `omp_simd.s` の中に積和演算のSIMD命令 `vfmadd...pd` (または `vmulpd` + `vaddpd`) が現れる.
命令末尾の `pd` は _packed double precision_ の略で, _p_ (packed) がSIMD命令であることの証しである.
また `-Minfo` の出力に "Generated vector ..." のようなベクトル化成功のメッセージが現れることを確認せよ.

指示行を追加する前後でアセンブリ (`omp_simd.s`) や `-Minfo` の出力がどう変わるかを比べてみよ.
