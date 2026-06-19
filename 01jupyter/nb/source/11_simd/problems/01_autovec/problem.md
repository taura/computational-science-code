# 練習問題: 自動ベクトル化のレポートを読む (C と Fortran の違い)

## 目標

コンパイラの最適化レポート (`-Minfo`) を読み, ループがSIMD化されたかどうかを自分で確認できるようになる.
とくに **C/C++ ではポインタの「エイリアスの可能性」が自動ベクトル化の妨げになる** のに対し, **Fortran は配列引数が重ならないと仮定するので何もしなくてもSIMD化されやすい** という違いを体験する.

これは穴埋め問題ではない. コンパイラの出力を読んで考える問題である.

## 課題

`autovec.cpp` (または `autovec.f90`) は saxpy (`y[i] = a*x[i] + y[i]`) を行う.

### 手順1: まず動かす

```
nvc++ -fast autovec.cpp -o autovec.exe   # C++
nvfortran -fast autovec.f90 -o autovec.exe  # Fortran
./autovec.exe
```

### 手順2: 最適化レポートを読む

`saxpy` 関数だけをオブジェクトコンパイル (`-c`) し, `-Minfo`/`-Mneginfo` のレポートと生成アセンブリ (`-Mkeepasm` で `.s`) を見る.

```
# C++
nvc++ -fast -Minfo -Mneginfo -Mkeepasm -c autovec.cpp
# Fortran
nvfortran -fast -Minfo -Mneginfo -Mkeepasm -c autovec.f90
```

- **C++** では, ループはベクトル化される一方で「エイリアスの可能性があるため版を分けた (versioned for ...)」旨の報告が出ることがある. これは, 実行時に `x` と `y` が重なっていないか確認してから速い版を使う, という安全策である.
- **Fortran** では, そうした版分けの注記なしに素直にベクトル化される (配列引数は重ならない前提のため).

`.s` の中に `pd` の付いた packed double 命令 (`vmulpd`, `vaddpd`, `vfmadd...pd`) が出ているかも確認せよ.

### 手順3 (工夫): C++ で版分けを無くす

C++ 版の `saxpy` のポインタに `__restrict` を付ける, または ループ直前に `#pragma omp simd` を加える (後者は `-mp=multicore` が必要) と, 「重ならない」とコンパイラに伝えられ, エイリアスのための版分けが消える. 前後で `-Minfo` の出力がどう変わるか比べよ.

```cpp
void saxpy(long n, double a, double * __restrict x, double * __restrict y) { ... }
```

## 期待される結果・考察

- C++ は (何も言わないと) エイリアスの可能性に備えるが, `__restrict` や `#pragma omp simd` で解消できる.
- Fortran は最初からその心配が無く, 自動ベクトル化される.
- 「同じ計算でも, 言語の前提 (エイリアスの扱い) によってコンパイラの仕事が変わる」ことを理解せよ.
