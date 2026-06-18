""" md

#* OpenMP入門(5) --- データの共有と `reduction`

# おさらい

* ここまでで `parallel` / `for` / `schedule` を使ってループを並列化し, 台数効果を測定できるようになった
* このトピックでは, OpenMPにおける<font color="blue">データの共有</font>と, それが引き起こす<font color="red">競合状態</font>, そしてその解決法 `reduction` を学ぶ
* このトピックで覚えるべきキーワード
  * `reduction(演算:変数)`

# 環境設定
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
# OpenMPにおけるデータの共有

* OpenMPのスレッドは基本的に全てのデータ(変数や配列)を<font color="blue">「共有」</font>している
* 「共有」しているとは, 大雑把にいえば, どのスレッドが変数に書き込んだ値も, 他のスレッドに見えるということである
* これまでの例題でも暗黙的に前提としていたことで, 例えば `parallel for` でループの各繰り返しが配列 `x[i]` に書き込んだ値を, 後で1スレッドに戻ってから表示できたのも, スレッドがデータを共有しているからである

* 一方で, スレッドごとに別々であってほしい変数もある
  * C/C++では, `parallel` 構文の中(`{ ... }` の内側)で宣言した変数は, スレッドごとに別の変数になる(共有されない)
  * Fortranでは, `private(変数)` 句で「この変数はスレッドごとに別物にする」と明示する
* データが共有されているのは便利でもある一方で, 気をつけないと<font color="red">競合状態</font>を生む

# 競合状態

* 以下のコードをOpenMPで並列化することを考える ($\int_0^1 1/(1+x^2)\,dx = \pi/4$ を数値積分する)

## C++版
"""

""" code w """
%%writefile integral.cpp
""" include nb/source/06_reduction/include/integral.cpp """
""" """

""" code w """
%%bash
nvc++ -fast -mp=multicore integral.cpp -o integral_mp.exe
""" """

""" code w """
%%bash
./integral_mp.exe
""" """

""" md
## Fortran版
"""

""" code w """
%%writefile integral.f90
""" include nb/source/06_reduction/include/integral.f90 """
""" """

""" code w """
%%bash
nvfortran -fast -mp=multicore integral.f90 -o integral_f_mp.exe
""" """

""" code w """
%%bash
./integral_f_mp.exe
""" """

""" md
* ここにそのまま `parallel for` (`parallel do`) を当てはめると以下のようになる
* (Fortran版では, 各スレッドで別の値を持つべき `x` を `private(x)` にしている. ただし `s` はわざと共有のままにしてある)

## C++版
"""

""" code w """
%%writefile omp_integral_racy.cpp
""" include nb/source/06_reduction/include/omp_integral_racy.cpp """
""" """

""" code w """
%%bash
nvc++ -fast -mp=multicore omp_integral_racy.cpp -o omp_integral_racy_mp.exe
""" """

""" md
## Fortran版
"""

""" code w """
%%writefile omp_integral_racy.f90
""" include nb/source/06_reduction/include/omp_integral_racy.f90 """
""" """

""" code w """
%%bash
nvfortran -fast -mp=multicore omp_integral_racy.f90 -o omp_integral_racy_f_mp.exe
""" """

""" md
* 以下ではいちいち示さないが参考のため Odyssey用のコンパイルオプション (C++ / Fortran)
```
FCCpx -Kfast -Kopenmp omp_integral_racy.cpp -o omp_integral_racy_mp.exe
frtpx -Kfast -Kopenmp omp_integral_racy.f90 -o omp_integral_racy_f_mp.exe
```
"""

""" md
* 正解は $\pi/4 \approx 0.785398$ で, 1スレッドで実行すれば問題なく正解が出る
"""

""" code w """
%%bash
OMP_NUM_THREADS=1 time ./omp_integral_racy_mp.exe
""" """

""" md
* 2スレッド以上だと正解は出ない上, 毎回答えも違う(非決定的な挙動)
"""

""" code w """
%%bash
OMP_NUM_THREADS=4 time ./omp_integral_racy_mp.exe
""" """

""" md
* その理由を考察する(講義スライド pXX 〜に答えがある)

<pre>
#pragma omp parallel for
  for (long i = 0; i < n; i++) {
    double x = a + i * dx;
    s += 1 / (1 + x * x);
  }
</pre>

において, 変数 `s` は全部のスレッドで共有されている(注: `x` は `parallel` で実行される文の内部で定義されており(Fortran版では `private`), そのような変数はスレッドごとに別の変数になる. つまり共有されない)
* 複数のスレッドが同じ変数`s`を更新することになる
* `s += ...` は実際には, `s` を読み出し, `...` を足して, それをまた`s`に書き戻す. 読み出しから書き戻しまでの間に別のスレッドが `s` を更新するとおかしなことになる

* このように, 複数のスレッドが同じ変数を使っていて, 少なくとも一人は更新している状況を<font color="red">「競合状態」</font>と呼び, 大概のプログラムは意図した動作をしない
* 競合状態があったら必ず何か手を打たないといけないと思っておくべき

## reduction 節を用いた解決法

* 今回の変数 `s` に対する足し込みのような演算に対してはとりわけ簡単な解決法
* `parallel` もしくは `for` (`do`) の中に `reduction(演算:変数)` という節を付け加えれば良い
* C/C++ も Fortran も書き方は同じ (`reduction(+:s)`)

## C++版
"""

""" code w """
%%writefile omp_integral_reduction.cpp
""" include nb/source/06_reduction/include/omp_integral_reduction.cpp """
""" """

""" code w """
%%bash
nvc++ -fast -mp=multicore omp_integral_reduction.cpp -o omp_integral_reduction_mp.exe
""" """

""" code w """
%%bash
OMP_NUM_THREADS=4 time ./omp_integral_reduction_mp.exe
""" """

""" md
## Fortran版
"""

""" code w """
%%writefile omp_integral_reduction.f90
""" include nb/source/06_reduction/include/omp_integral_reduction.f90 """
""" """

""" code w """
%%bash
nvfortran -fast -mp=multicore omp_integral_reduction.f90 -o omp_integral_reduction_f_mp.exe
""" """

""" code w """
%%bash
OMP_NUM_THREADS=4 time ./omp_integral_reduction_f_mp.exe
""" """

""" md
* reductionは一般に多数の値を縮約する演算 o ($s = s_0 {\rm o} s_1 {\rm o}  ... {\rm o} s_{n-1}$)であって, 適用する順番を変えても同じ答えが出る場合に使われる
* つまり足し算(+)であれば $s_0 + s_1 + s_2 + s_3$を,
$$(((s_0 + s_1) + s_2) + s_3)$$
と計算しても良いし
$$(s_0 + s_1) + (s_2 + s_3)$$
と計算しても良い.
* 見ての通り後者のように計算すれば $(s_0 + s_1)$ と $(s_2 + s_3)$ を並行に計算できることになる
* したがって reduction は, 結合則と交換則が成り立つような演算を多数の要素に施す場合に使え, 代表例としては, +, *, max, min, などがある

* ただし実は, 浮動小数点数(`double`, `float`など)の足し算には丸め誤差が生ずる
* そのため浮動小数点数を多数足し合わせる場合, その順番により結果が変わりうる
* `reduction`を使った場合, 1スレッドの実行と答えがずれる, スレッド数によって答えが少し変わるということは普通に生ずる

# 終わりに一言

* これでOpenMPを用いて, マルチコア(複数のコアを使った)並列処理ができるようになった
* ただしこれだけでCPUの性能をどのくらい引き出せるかと言うと, 本来性能には程遠い
* その理由は主にSIMD命令を用いていないこと, 命令間の依存関係によって, 命令レベル並列を引き出せていないこと, である(後のトピックで扱う)
* また, 次のトピックからはGPUを使った並列化を扱う
"""
