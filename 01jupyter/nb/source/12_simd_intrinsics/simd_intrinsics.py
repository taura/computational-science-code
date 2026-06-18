""" md

#* CPU SIMD入門(2) --- ベクトル型拡張による明示的SIMD

# おさらい

* 前のトピックでは, SIMD命令とは何か, そして自動ベクトル化や `#pragma omp simd` (`!$omp simd`) によってコンパイラにSIMD化を促す方法を学んだ
* しかし, 自動ベクトル化や `omp simd` は<font color="red">期待したようにSIMD化してくれないことが多い</font>
  * SIMD化に成功しても256 bit命令しか使ってくれない, 思ったほど速くならない, そもそもSIMD化を諦めてしまう, など
  * うまくいかない理由をコンパイラのメッセージから読み解くのも難しい
* このトピックでは, より<font color="blue">確実で明示的</font>にSIMD命令を使う方法 --- <font color="blue">ベクトル型拡張</font> --- を学ぶ
* ベクトル型を使うと, 複数の値を束ねたデータと, それに対する演算をプログラマが明示的に書くことができ, ほぼ確実にSIMD命令に変換される

* このトピックで覚えるべきキーワード
  * ベクトル型 (`__attribute__((vector_size(N)))`)
  * スカラ型 <-> ベクトル型の変換, `loadv` / `storev`

# 環境設定

* Jupyter上でコンパイラを起動する, およびジョブ投入を簡便にするための設定
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
# SIMDプログラミングの方法のおさらい

* ベクトル化(SIMD命令を利用する)にはいくつかの方法がある

1. **自動SIMD化:** プログラマはSIMD命令のことをあまり意識せず普通のループを書き, コンパイラがそれをSIMD命令に変換してくれることを期待する(前のトピック)
1. **OpenMP `omp simd`:** 上記と実質的に同じだが, 実行の順番を入れ替えても良いことをプログラマが明示的にコンパイラに伝える(前のトピック)
1. **ベクトル型拡張とintrinsics:** 複数の値をまとめたデータやそれに対する演算を明示的に扱う(<font color="blue">このトピック</font>)
1. **アセンブリ言語:** 自分でアセンブリ言語を書く(最後の手段)

* 1.や2.で話がすむのが理想だが, 実際のところはなかなかそうはいかない. 色々工夫をしてもなかなかうまくいかない, 速くならないとイライラすることになる
* 以下では多少手間はかかるものの, 努力に対する効果がある程度保証されやすい3.の方法について述べる

# ベクトル型拡張

## ベクトル型の定義

* ベクトル型は複数の要素を束ねた一つの値 --- Single Instruction Multiple Dataの "Multiple Data" に相当する値 --- を表す型である
* GCC, Clang, NVIDIA(nvc++)などのコンパイラでは, `__attribute__((vector_size(N)))` という記法でユーザがそのような型を定義できる(あまり知られていない機能)
* `N` は<font color="blue">バイト単位</font>での型全体の大きさ. 例えば `double` (8バイト)を8つまとめると 8 × 8 = 64 バイト (= 512 bit)
* 例えば以下で `double`を 8つ (512 bit分) まとめた一つのベクトル型 `doublev` を定義している
* なお, ベクトル型にする前の型(`double`など)を区別して, <font color="blue">スカラ型</font>と呼ぶことがある

* 注: `__attribute__((vector_size(N)))` は<font color="red">C/C++独自の拡張</font>であり, Fortranには相当する機能が無い(後述の「Fortranでは」節を参照)
"""

""" code w """
%%writefile doublev.cpp
""" include nb/source/12_simd_intrinsics/include/doublev.cpp """
""" """

""" code w """
%%bash
nvc++ -fast -Mkeepasm -Minfo -Mneginfo -c doublev.cpp
""" """

""" md
## ベクトル型に対する演算

* そのようにして定義した型は `int`, `double` などと同様, 変数(関数の引数を含む)や関数返り値の型として使える
* また, (ここが肝心だが), `+` や `*` などの見慣れた記法で演算が書け, それらは<font color="blue">ほぼ確実に</font>SIMD命令に変換される
* 以下は `double` 8つ分の掛け算と加算をまとめて行う(いわゆる fma, fused multiply-add)関数である
* 一見して普通の `double` を対象とする関数と非常に似ていることに注意
"""

""" code w """
%%writefile doublev_fma.cpp
""" include nb/source/12_simd_intrinsics/include/doublev_fma.cpp """
""" """

""" code w """
%%bash
nvc++ -fast -Mkeepasm -Minfo -Mneginfo -c doublev_fma.cpp
""" """

""" md
* 生成されたアセンブリを確認する
"""

""" code w """
%%bash
cat doublev_fma.s
""" """

""" md
* 関係するところだけを抜粋すると以下
```
doublev_fma:                            # @doublev_fma
	vfmadd213pd	%zmm2, %zmm1, %zmm0     # zmm0 = (zmm1 * zmm0) + zmm2
	retq
```
* `%zmm`という512 bitのレジスタ --- つまり512 bitのSIMD命令 --- が使われている
* `vfmadd...pd` は掛け算と足し算を1命令で行うfma命令で, 末尾の `pd` は _packed double precision_, すなわち `double` のSIMD演算であることを示す
* 前のトピックの自動ベクトル化では256 bitの命令(`%ymm`)しか使われない例があったが, ベクトル型を明示的に使うことで, 望み通りの512 bit命令を確実に引き出せている
* これがベクトル型拡張を使う大きな理由のひとつである

* また, 普通の `double` 型(スカラ型)と `doublev` 型(ベクトル型)を混ぜた演算も行うことができる
"""

""" code w """
%%writefile doublev_fma_mixed.cpp
""" include nb/source/12_simd_intrinsics/include/doublev_fma_mixed.cpp """
""" """

""" code w """
%%bash
nvc++ -fast -Mkeepasm -Minfo -Mneginfo -c doublev_fma_mixed.cpp
""" """

""" code w """
%%bash
cat doublev_fma_mixed.s
""" """

""" md
* 関係するところを抜粋すると以下
```
doublev_fma_mixed:                      # @doublev_fma_mixed
	vbroadcastsd	%xmm0, %zmm0
	vfmadd213pd	%zmm2, %zmm1, %zmm0     # zmm0 = (zmm1 * zmm0) + zmm2
	retq
```
* `vbroadcastsd %xmm0, %zmm0` は1つの `double` 型の値(`%xmm0`)を8つに複製して `%zmm0` に格納する命令
* このように, スカラ型の値はベクトル型と演算する際, 自動的に「全要素同じ値のベクトル」に拡げられる(broadcast)

# スカラ型 &lt;-&gt; ベクトル型

* SIMD化したいループでは, 「繰り返しごとに異なる値」をベクトル型で持ち, 「繰り返しによらず一定の値(スカラ)」をベクトル型に変換したり, 配列とベクトル型の間でデータを出し入れしたりする必要がある
* そのための基本操作を見ていく

## スカラ型からベクトル型を作る (uniform)

* 要素を明示的に並べる文法としてはCの配列と同様の文法 `{ a, b, c, ... }` が使える
* 以下は同じ要素(`u`) 8つからなるベクトル型の値を作る関数
"""

""" code w """
%%writefile uniform.cpp
""" include nb/source/12_simd_intrinsics/include/uniform.cpp """
""" """

""" code w """
%%bash
nvc++ -fast -Mkeepasm -Minfo -Mneginfo -c uniform.cpp
""" """

""" code w """
%%bash
cat uniform.s
""" """

""" md
* 関係するところを抜粋すると以下
```
uniform:                                # @uniform
	vbroadcastsd	%xmm0, %zmm0
	retq
```
* やはり `vbroadcastsd` 1命令にコンパイルされている

## 配列からベクトル型に読み込む (loadv)

* 以下は `double` の配列 `a` の `a[0]`〜`a[7]` の8要素を取り出してひとつのベクトル型の値とする
* ポインタを `doublev*` にキャストして間接参照するだけで, 連続する8要素をまとめて読み込める
"""

""" code w """
%%writefile loadv.cpp
""" include nb/source/12_simd_intrinsics/include/loadv.cpp """
""" """

""" code w """
%%bash
nvc++ -fast -Mkeepasm -Minfo -Mneginfo -c loadv.cpp
""" """

""" code w """
%%bash
cat loadv.s
""" """

""" md
* 関係するところを抜粋すると以下
```
loadv:                                  # @loadv
	vmovups	(%rdi), %zmm0
	retq
```
* 全体が一つのSIMDロード命令になっている

## ベクトル型から配列へ書き込む (storev)

* 以下は逆にベクトル型の値を `double` の配列 `a` の `a[0]`〜`a[7]` の8要素にセットする
"""

""" code w """
%%writefile storev.cpp
""" include nb/source/12_simd_intrinsics/include/storev.cpp """
""" """

""" code w """
%%bash
nvc++ -fast -Mkeepasm -Minfo -Mneginfo -c storev.cpp
""" """

""" code w """
%%bash
cat storev.s
""" """

""" md
* 512 bitの値を一度に, あるいは半分ずつに分けて格納するSIMDストア命令が使われる(コンパイラのバージョンにより `vmovups`/`vmovupd` 一発になったり, 半分ずつに分かれたりする)

# Fortranでは

* <font color="red">重要:</font> ここまで紹介した `__attribute__((vector_size(N)))` によるベクトル型は<font color="red">C/C++独自の拡張</font>であり, Fortranには直接相当する機能が無い
* Fortranで明示的・確実にSIMDを使いたい場合の現実的な選択肢は以下である
  1. 前のトピックで学んだ `!$omp simd` 指示行を使い, ループのSIMD化をコンパイラに明示的に促す
  1. <font color="blue">配列構文(whole-array expression)</font> `z = a*x + y` のように配列全体に対する演算を書き, コンパイラの自動ベクトル化に委ねる
* どちらの場合も「確実に望みの幅のSIMD命令が出る」という保証はC/C++のベクトル型ほど強くない. 出力されたアセンブリやコンパイラメッセージ(`-Minfo`)で確認することが大切である

* 以下にFortranでの書き方の例を示す($z_i = a x_i + y_i$ の計算)
"""

""" code w """
%%writefile simd_fortran.f90
""" include nb/source/12_simd_intrinsics/include/simd_fortran.f90 """
""" """

""" code w """
%%bash
nvfortran -fast -Mkeepasm -Minfo -Mneginfo -c simd_fortran.f90
""" """

""" md
* `axpy` は `!$omp simd` を使ったループ, `axpy_array` は配列構文 `z = a*x + y` を使ったもの
* `-Minfo` の出力に "Generated vector ..." のようなメッセージが出ていればSIMD化に成功している

# まとめ --- ベクトル型を使ったプログラミングの要諦

* ベクトル化したいループが与えられたとき, 連続する `nl` 回(512 bit / 64 bit = 8 など)の繰り返しを<font color="blue">まとめて一度に実行する</font>のが目標
* そのために,
  * 繰り返しごとに異なる値を持つ式は, 極力<font color="blue">ベクトル型のまま</font>計算する
  * 繰り返しによらず一定の値(スカラ)は, 必要に応じて `uniform` などでベクトル型に拡げる
  * 配列との出し入れには `loadv` / `storev` を使う
* 擬似的に書けば, 元のループ
```
for (long i = 0; i < m; i++) {
  x[i] = f(i);
}
```
を
```
for (long i = 0; i < m; i += nl) {       // nl 回ずつまとめて
  storev(&x[i], f_vec(i));                 // f_vec は f をベクトル型で計算する版
}
```
のように書き換える, ということである
* C/C++ではこれをベクトル型で明示的・確実に実現できる. Fortranでは `!$omp simd` / 配列構文 + 自動ベクトル化に頼ることになる
"""
