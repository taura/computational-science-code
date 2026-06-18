# 練習問題: ベクトル型でfma (積和) を書く

## 目標

ベクトル型 (`__attribute__((vector_size(N)))`) の値に対して, 普通の `double` と同じ `*` `+` の記法で演算を書くと, ほぼ確実にSIMD命令 (`vfmadd...pd` など) に変換されることを体験する.

## 課題

### C++ (ベクトル型)

`vector_fma.cpp` には, `double` 8つ分 (512 bit) のベクトル型 `doublev` を引数にとり, 積和 (fma, fused multiply-add) `a * b + c` を計算する関数 `vector_fma` がある.
本体が `TODO` で空になっている.

コメント `TODO` の指示に従って, **`a * b + c` を返す1行を書け**.

```cpp
return a * b + c;
```

a, b, c はいずれもベクトル型なので, `*` `+` は要素ごとのSIMD演算になる. それ以外のコードを変更する必要はない.

### Fortran (`!$omp simd`)

<font color="red">重要:</font> `__attribute__((vector_size(N)))` によるベクトル型は **C/C++独自の拡張** であり, Fortranには相当する機能が無い.
そこでFortran版 `vector_fma.f90` では, 同じ `y(i) = a(i) * b(i) + c(i)` の計算をループで書き, `!$omp simd` でSIMD化する.
`TODO` の指示に従って doループの直前に **`!$omp simd` を1行追加** せよ.

## コンパイルと実行

```
# C++ (ベクトル型)
nvc++ -fast -Mkeepasm -Minfo -Mneginfo -c vector_fma.cpp

# Fortran (!$omp simd を解釈させるため -mp=multicore が必要)
nvfortran -fast -mp=multicore -Mkeepasm -Minfo -Mneginfo -c vector_fma.f90
```

生成されたアセンブリを確認する.

```
cat vector_fma.s
```

## 期待される結果

C++版では `vector_fma.s` の中に, 512 bit レジスタ `%zmm` を使った積和命令

```
vfmadd213pd %zmm2, %zmm1, %zmm0   # zmm0 = (zmm1 * zmm0) + zmm2
```

が現れる. `pd` は _packed double precision_ の略である.
自動ベクトル化では256 bit命令 (`%ymm`) しか使われない例があったが, ベクトル型を明示的に使うことで望み通りの512 bit命令を確実に引き出せていることを確認せよ.

Fortran版では, `-Minfo` の出力に "Generated vector ..." のようなメッセージが現れ, `vector_fma.s` に `pd` 付きのSIMD命令が生成されていればSIMD化に成功している.
