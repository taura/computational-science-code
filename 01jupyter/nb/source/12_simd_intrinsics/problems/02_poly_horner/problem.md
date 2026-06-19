# 練習問題: ベクトル型で多項式を Horner 法で評価する

## 目標

ベクトル型 (`vector_size`) を使い, 多項式
`p(x) = c0 + c1*x + c2*x^2 + c3*x^3`
を配列の各要素について評価する. 計算には乗算と加算を交互に繰り返す **Horner 法** を使う:

```
acc = c3
acc = acc*x + c2
acc = acc*x + c1
acc = acc*x + c0
```

これを `nl = 8` レーンのベクトルのまま (8要素同時に) 行うのがポイントである. 各ステップ `acc = acc*x + c_k` は積和 (FMA) であり, ベクトル型で書けばコンパイラが `vfmadd...pd` のような packed double のFMA命令を生成する.

## 課題

`poly_horner.cpp` は, `x[i]` を `nl` 個ずつベクトル `xv` に読み込むところまで用意してある.
コメント `TODO` の指示に従い, **ベクトル変数 `acc` を使って Horner 法で多項式を評価**し, `acc` に `p(x)` を求めよ (係数の数だけ `acc = acc*xv + c[k]` を繰り返す).

- C++: `acc` を最高次の係数 `c[deg]` で初期化し, `k` を減らしながら `acc = acc * xv + c[k]` を繰り返す.
- Fortran: ベクトル型は無いので, 普通の `do` ループで1要素ずつ Horner 法を書く (自動ベクトル化に任せる).

## コンパイルと実行

```
# C++ (ベクトル型はプレーンなコンパイラでも使える. ここでは最適化を効かせる)
nvc++ -fast poly_horner.cpp -o poly_horner_cpp.exe
./poly_horner_cpp.exe

# Fortran
nvfortran -fast poly_horner.f90 -o poly_horner_f90.exe
./poly_horner_f90.exe
```

`n` はコマンドライン引数で指定でき (既定 64, `nl=8` の倍数), 結果はスカラ版の計算と照合される.

余裕があればアセンブリを確認せよ (`nvc++ -fast -Mkeepasm poly_horner.cpp ...` で `.s` が残る). Horner 法の各段が `vfmadd...pd` (packed double のFMA) になっていれば成功である.

## 期待される結果

- `OK: p[0]=1.000, p[63]=...` のように, ベクトル計算の結果がスカラ版と一致する.
- 実装が抜けていると `acc` が未定義のまま書き戻され, `NG` と表示される.
