# 練習問題: omp simd reduction で内積をSIMD化する

## 目標

総和をとるループ (リダクション) を `#pragma omp simd reduction(+:s)` (Fortran では `!$omp simd reduction(+:s)`) でSIMD化する.
00 の saxpy のような要素ごと独立の演算 (elementwise) と違い, 内積は1つの変数 `s` に値を足し込んでいくため, そのままでは各反復に依存関係があり自動ベクトル化されにくい. `reduction` 節で「部分和を複数レーンに分けて最後に合計してよい」とコンパイラに伝えるのがポイントで, 00 より一歩進んだ内容である.

## 課題

`dot_simd.cpp` (または `dot_simd.f90`) は内積 `s = Σ x[i]*y[i]` を計算する関数 `dot` を持つ.
コメント `TODO` の指示に従って **OpenMP の指示行を1つ追加** し, 総和ループがSIMD化されるようにせよ.

- C++: `for` ループの直前に `#pragma omp simd reduction(+:s)` を1行加える.
- Fortran: `do` ループの直前に `!$omp simd reduction(+:s)` を1行加える.

それ以外のコードを変更する必要はない.

## コンパイルと実行

`#pragma omp simd` を解釈させるには `-mp=multicore` が必要である (SIMD命令を使うだけなので1スレッドで動作する).

```
# C++
nvc++ -fast -mp=multicore -Mkeepasm -Minfo -Mneginfo dot_simd.cpp -o dot_simd_cpp.exe
./dot_simd_cpp.exe

# Fortran
nvfortran -fast -mp=multicore -Mkeepasm -Minfo -Mneginfo dot_simd.f90 -o dot_simd_f90.exe
./dot_simd_f90.exe
```

`x[i]=1`, `y[i]=2` としているので, 内積は理論値 `2*n` になり, `OK` と表示されれば正しい.

生成されたアセンブリ (`dot_simd.s`) を確認すると, 積和のSIMD命令 (`vfmadd...pd` など packed double 命令) と, 最後に複数レーンの部分和をまとめる水平加算 (horizontal add) が現れる.

## 期待される結果

- `OK: s=200000000.0 (= 2*n)` のように, 内積が理論値 `2*n` に一致する.
- `dot_simd.s` の中に `pd` 付きの packed double 命令が現れ, `-Minfo` に "Generated vector ..." が出る.
- Fortran では組込み関数 `dot_product(x, y)` を使っても同様にベクトル化される (余裕があれば比べてみよ).
