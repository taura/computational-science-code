""" md

#* CPU SIMD入門(1) --- 自動ベクトル化と omp simd 指示

# SIMD命令とは

* SIMDはSingle Instruction Multiple Dataの略
* その名の通り1命令 (Single Instruction) で多数のデータ (Multiple Data) に対して同一の演算を行う命令
* 最近のIntel CPUは最大で512 bit, 具体的には
  * 32 bit (4バイト)データ (`float`) $\times$ 16個
  * 64 bit (8バイト)データ (`double`) $\times$ 8個
に対して演算を行う命令を備えている
* 「同じ命令を異なるデータに対して施す」という状況が典型的に現れるのは, やはりforループ(Fortranではdoループ)である
* forループの連続する何回(例えば8回や16回)かの繰り返しをSIMD命令を用いて同時に実行する
* これを**SIMD化**または**ベクトル化**と呼ぶ
  * SIMD命令を使うのだからSIMD化と呼ぶのが自然だが, 歴史的な理由でベクトル化と呼ぶことが多い

* このトピックで覚えるべきキーワード
  * 自動ベクトル化
  * `#pragma omp simd` (Fortran: `!$omp simd`)

# 環境設定

* Jupyter上でコンパイラを起動するための設定
* これは各Jupyterノートブックごとに行う
* 同じノートブックでもログアウトしたりカーネルを再スタートしたときなどは失われるのでそのたびに行うこと
"""

""" code w """
import os
paths = os.environ["PATH"].split(":")
nvc_path = "/work/opt/local/x86_64/cores/nvidia/23.3/Linux_x86_64/23.3/compilers/bin"
fj_path = "/opt/FJSVxtclanga/tcsds-1.2.41/bin"
for path in [nvc_path, fj_path]:
    if path not in paths:
        paths = [path] + paths
os.environ["PATH"] = ":".join(paths)
""" """

""" code w """
%%bash
which nvc++
which nvfortran
""" """

""" code """
import wisteria_submit
""" """

""" code """
import heytutor
""" """

""" md
# 自動ベクトル化

* 以下のような単純なループは, プログラマが特別なことをしなくてもコンパイラがベクトル化(SIMD命令に変換)してくれる
* これを**自動ベクトル化**と呼ぶ
* 以下は配列の要素ごとの足し算 `z[i] = x[i] + y[i]` を8要素について行う関数である

## C++版
"""

""" code w """
%%writefile add.cpp
""" include nb/source/11_simd/include/add.cpp """
""" """

""" md
* 実際にどのようなコードにコンパイルされたかは, 以下のようなコマンドで確認できる
"""

""" code w """
%%bash
nvc++ -fast -Mkeepasm -Minfo -Mneginfo -c add.cpp
""" """

""" md
* `-c` : 実行可能ファイルを生成せずオブジェクトファイル(`add.o`)まででコンパイルをやめる(`main`関数が存在しないため)
* `-Mkeepasm` : その上で生成されたアセンブリ言語のファイル(`add.s`)を残す
* `-Minfo` / `-Mneginfo` : コンパイラがSIMD化(ベクトル化)に成功した・失敗したことを報告してくれる

* 生成されたアセンブリコード `add.s` を確認する
"""

""" code w """
%%bash
cat add.s
""" """

""" md
* 関数名`add`に対応して, `add:` というラベルから始まる命令列が生成された命令列である
* `retq` が関数の終了で, その前後の重要な部分を抜粋すると以下のようになっている
```
add:                                    # @add
	vmovupd	(%rsi), %ymm0
	vaddpd	(%rdi), %ymm0, %ymm0
	vmovupd	%ymm0, (%rdx)
	vmovupd	32(%rsi), %ymm0
	vaddpd	32(%rdi), %ymm0, %ymm0
	vmovupd	%ymm0, 32(%rdx)
	vzeroupper
	retq
```
  * `vmovupd (%rsi), %ymm0` : SIMDロード命令(メモリ上の複数の`double`をレジスタに読み込む)
  * `vaddpd (%rdi), %ymm0, %ymm0` : SIMD加算命令(複数の`double`を一度に足す)
  * `vmovupd %ymm0, (%rdx)` : SIMDストア命令(レジスタの複数の`double`をメモリに書き戻す)
* なお,
  * 命令の最後の2文字 `pd` は _packed double precision_ の略であり, _p_ (packed) がSIMD命令であることの証し, _d_ は64 bitの浮動小数点(`double`)が対象であることの証し
  * 命令に使われているレジスタ(`%ymm0`)は256 bitのレジスタであり, `double` であれば4つ分. もともとのコードは8要素に対する演算だったので, それぞれの命令が2回ずつ行われている
* 擬似的にはもともとのループを以下のように実行していることに相当する
```
z[0:4] = x[0:4] + y[0:4]; // SIMD命令
z[4:8] = x[4:8] + y[4:8]; // SIMD命令
```

## Fortran版
* 同じ内容をFortranで書くと以下のようになる
"""

""" code w """
%%writefile add.f90
""" include nb/source/11_simd/include/add.f90 """
""" """

""" code w """
%%bash
nvfortran -fast -Mkeepasm -Minfo -Mneginfo -c add.f90
""" """

""" md
* C++版と同様にアセンブリコード `add.s` を確認すると, `vaddpd` などのSIMD命令が生成されていることがわかる(関数名はFortranの規則により `add_` のようになっていることに注意)
"""

""" code w """
%%bash
cat add.s
""" """

""" md
* 一般にforループ(doループ)をベクトル化するにあたってコンパイラが試みることを擬似的に書くと,
```
for (i = 0; i < n; i++) {
  A(i);
  B(i);
   ...
}
```
というfor文 (`A(i), B(i)` は `i` を含んだ任意の文ということであり, 必ずしも関数呼び出しという意味ではない) を
```
for (i = 0; i < n; i += nl) {
  A(i:i+nl);
  B(i:i+nl);
    ...
}
```
のように実行することである(`nl`はSIMD命令で一度に実行できる繰り返しの数で, 2, 4, 8, 16など)
* なおこの実行が正しい(SIMD命令を使わない場合と同じ結果になる)ためには, **繰り返しの実行順序を入れ替えてもよい**という条件が必要で, それは並列化が合法な条件とほぼ同じだと考えれば良い
"""

""" md
# SIMDプログラミングの方法

* ベクトル化(SIMD命令を利用する)にはいくつかの方法がある

1. **自動ベクトル化:** プログラマはSIMD命令のことをあまり意識せず普通のループを書き, コンパイラがそれをSIMD命令に変換してくれることを期待する(上で見た方法)
1. **OpenMP `#pragma omp simd` (Fortran: `!$omp simd`):** 上記と実質的に同じだが, 実行の順番を入れ替えても良いことをプログラマが明示的にコンパイラに伝える(このトピックの後半で扱う)
1. **ベクトル型拡張とintrinsics:** 複数の値をまとめたデータやそれに対する演算を明示的に扱う(**次のトピックで扱う**)
1. **アセンブリ言語:** 自分でアセンブリ言語を書く

* 1.や2.で話がすむのが理想だが, 実際のところはなかなかそうはいかない. 色々工夫をしてもなかなかうまくいかない, 速くならないとイライラすることになる
* 4.は最後の手段
* このトピックでは1.と2.を扱う. 3.(ベクトル型拡張とintrinsics)は次のトピックで扱う
"""

""" md
# `#pragma omp simd` / `!$omp simd`

* 自動ベクトル化は便利だが, コンパイラは「繰り返しの順序を入れ替えても安全か」を自分で判断できない場合がある
  * 例えば配列の要素が重なっている(エイリアスしている)可能性があると, 安全のためベクトル化を諦めてしまうことがある
* `#pragma omp simd` (Fortran: `!$omp simd`) は, 直後のループについて「繰り返しを入れ替えて(まとめて)実行してよい」ことを**プログラマが明示的にコンパイラに伝える**指示である
* これにより, 自動ベクトル化では諦めていたループもSIMD化される可能性が高まる
* 書き方
  * C/C++: ループの直前に `#pragma omp simd`
  * Fortran: doループの直前に `!$omp simd`
* 以下は `y[i] = a * x[i] + y[i]` (saxpy/axpyと呼ばれる典型的な演算)を `#pragma omp simd` でベクトル化する例である

## C++版
"""

""" code w """
%%writefile omp_simd.cpp
""" include nb/source/11_simd/include/omp_simd.cpp """
""" """

""" md
* `#pragma omp simd` を含むOpenMP指示行を解釈させるには, 最適化オプションに加えてOpenMPを有効にするオプションが必要である
  * `-mp=multicore` はマルチコア並列(`parallel for`など)のためのオプションだが, これを付けておけば `#pragma omp simd` も解釈される
  * (`simd` 指示自体はスレッドを使わずSIMD命令を使うだけなので, `-fast` による自動ベクトル化と同様に1スレッドで動作する)
"""

""" code w """
%%bash
nvc++ -fast -mp=multicore -Mkeepasm -Minfo -Mneginfo -c omp_simd.cpp
""" """

""" code w """
%%bash
cat omp_simd.s
""" """

""" md
* `vfmadd...pd` (積和演算のSIMD命令)などが生成されていれば, ループがSIMD化されている

## Fortran版
"""

""" code w """
%%writefile omp_simd.f90
""" include nb/source/11_simd/include/omp_simd.f90 """
""" """

""" code w """
%%bash
nvfortran -fast -mp=multicore -Mkeepasm -Minfo -Mneginfo -c omp_simd.f90
""" """

""" code w """
%%bash
cat omp_simd.s
""" """

""" md
* 以下ではいちいち示さないが参考のため Odyssey(富士通コンパイラ)用のコンパイルオプション (C++ / Fortran)
```
FCCpx -Kfast -Kopenmp -c omp_simd.cpp
frtpx -Kfast -Kopenmp -c omp_simd.f90
```

# まとめ

* SIMD命令は1命令で複数のデータに同じ演算を施す命令で, ループの複数回の繰り返しを一度に実行することでベクトル化(SIMD化)する
* 単純なループはコンパイラが**自動ベクトル化**してくれる. 生成されたアセンブリ(`*.s`)を見ると `pd` 付きのSIMD命令や `%ymm`/`%zmm` レジスタが使われていることが確認できる
* 自動ベクトル化が効かない場合, `#pragma omp simd` (`!$omp simd`)で「繰り返しを入れ替えてよい」ことを明示するとSIMD化が促される
* より確実にSIMD命令を使うための**ベクトル型拡張とintrinsics**は次のトピックで扱う
"""
