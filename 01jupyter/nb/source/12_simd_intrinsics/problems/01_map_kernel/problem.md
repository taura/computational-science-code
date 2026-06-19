# 練習問題: ベクトル型で配列カーネルを書く

## 目標

配列の各要素に同じ計算 `y[i] = 2*x[i] + 1` を施すループを, ベクトル型を使って **`nl` 個ずつまとめて** 計算する方法を身につける. 「読み込み (load) → ベクトル演算 → 書き戻し (store)」という SIMD プログラミングの基本形を体験する.

## 課題

### C++ (ベクトル型)

`map_kernel.cpp` は, `double` を `nl=8` 個束ねたベクトル型 `doublev` を使い, 配列を `nl` 個ずつ処理する.
各かたまりについて `x[i..i+nl-1]` をベクトル `xv` として読み込み済み, 結果を `y[i..i+nl-1]` に書き戻すところも書いてある.
真ん中の **ベクトル計算が `TODO` で空** なので, 現状の `y` は不定値になり検算に失敗する.

`TODO` の箇所に **`yv = 2*x + 1` をベクトルのまま計算する1行** を書け.

```cpp
yv = 2.0 * xv + 1.0;
```

`xv` はベクトル型なので, `*` `+` は要素ごとのSIMD演算になる (スカラの `2.0`, `1.0` は自動的に全要素へ broadcast される).

### Fortran (配列演算)

ベクトル型は C/C++ 独自の拡張で Fortran には無い. `map_kernel.f90` では普通のループで `y(i) = 2*x(i) + 1` を書く (`TODO` を埋める). このような配列演算は Fortran のコンパイラが自動的にSIMD化する.

## コンパイルと実行

```
# C++
nvc++ -fast map_kernel.cpp -o map_kernel.exe

# Fortran
nvfortran -fast map_kernel.f90 -o map_kernel.exe
```

```
./map_kernel.exe 64
```

(余裕があれば `nvc++ -fast -Mkeepasm -Minfo -c map_kernel.cpp` で `.s` を見て, `vfmadd...pd` / `vmulpd` などの packed double 命令が出ていることを確認せよ.)

## 期待される結果

```
OK: y[0]=1.0, y[63]=127.0
```

`TODO` を埋める前は `y` が不定値になり `NG` と表示される (計算できていないことの確認).
