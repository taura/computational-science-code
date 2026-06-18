""" md 

#* 高性能プログラミングと性能測定(4) --- CPU SIMDプログラミング

"""

""" md
# 環境設定

* Jupyter上でコンパイラを起動する, およびジョブ投入を簡便にするための設定
* これは各Jupyterノートブックごとに行う
* 同じノートブックでもログアウトしたりカーネルを再スタートしたときなどは失われるのでそのたびに行うこと

## コンパイラ

* Aquariusでは, 同じコンパイラでCPUもGPUもサポートしているという理由で, NVIDIA HPC SDKを使う
  * コマンド名:
    * C: `nvc`
    * C++: `nvc++`
  * コンパイルオプション:
    * `-mp=multicore` をつけると CPU用のOpenMPがサポートされる
    * `-mp=gpu` をつけると GPU用のOpenMPがサポートされる
* Odysseyでは, 富士通コンパイラを使う
  * コマンド名:
    * C: `fccpx`
    * C++: `FCCpx`
  * コンパイルオプション:
    * `-Kopenmp` をつけると CPU用のOpenMPがサポートされる
* 上記のコマンドを実行できるようにするために, 以下を実行する
  * なお以下はコマンドライン端末上では `module load nvidia`, `module load fj` とするのが本来のやり方だがJupyter上で`module`コマンドが動かないのでやむなく以下のようにする
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

""" md

## ジョブ投入を簡便に行う設定

"""

""" code """
import wisteria_submit
""" """

""" md

# AIチューター

"""

""" code """
import heytutor
""" """

""" md 

# SIMD命令とは

* SIMDはSingle Instruction Multiple Dataの略
* その名の通り1命令 (Single Instruction) で多数のデータ (Multiple Data) に対して同一の演算を行う命令
* 最近のIntel CPUは最大で512 bit, 具体的には
  * 32 bit (4バイト)データ $\times$ 16個
  * 64 bit (8バイト)データ $\times$ 8個
に対して演算を行う命令を備えている
* 「同じ命令を異なるデータに対して施す」という状況が典型的に現れるのは, やはりforループである
* forループの連続する何回(例えば8回や16回)かの繰り返しをSIMD命令を用いて同時に実行する
* これを**SIMD化**または**ベクトル化**と呼ぶ
  * SIMD命令を使うのだからSIMD化と呼ぶのが自然だが歴史的な理由でベクトル化と呼ぶことが多い
* 以下のような単純なループは, コンパイラがベクトル化してくれる
"""

""" code w """
%%writefile add.c
""" exec-include ./mk_version.py -D VER=add nb/source/cs04/include/example.c """
""" """

""" md 
* 実際にどのようなコードにコンパイルされたかは, 以下のようなコマンドで確認できる
"""

""" code w """
%%bash
nvc -fast -Mkeepasm -Minfo -Mneginfo -c add.c
""" """

""" md 
* `-c` : 実行可能ファイルを生成せずオブジェクトファイル(`example_add.o`)まででコンパイルをやめる(`main`関数が存在しないため)
* `-Mkeepasm` : その上で生成されたアセンブリ言語のファイル(`example_add.s`, `example_add.ll`)を残す
* また, 以下のオプションではコンパイラがSIMD化に成功した, 失敗したことを報告してくれる
  * `-Minfo`
  * `-Mneginfo`
"""


""" md 
* 生成されているファイルの確認(`example_add.s`がアセンブリコード)
"""

""" code w """
%%bash
ls
""" """

""" md 
* `add.s` アセンブリコードの確認
"""

""" code w """
%%bash
cat add.s
""" """

""" md 
* 関数名`add`に対応して, `add:` というラベルから始まる命令列が生成された命令列
* `retq` が関数の終了で, そこまでを抜粋すると以下
```
add:                                    # @add
.Lfunc_begin0:
	.cfi_sections .debug_frame
	.cfi_startproc
# %bb.0:                                # %L.entry
	.file	1 "/home/csi/computational-science-code/01jupyter/notebooks/source/cs04/add.c"
	.loc	1 4 1 prologue_end              # add.c:4:1
	vmovupd	(%rsi), %ymm0
	vaddpd	(%rdi), %ymm0, %ymm0
	vmovupd	%ymm0, (%rdx)
	vmovupd	32(%rsi), %ymm0
	vaddpd	32(%rdi), %ymm0, %ymm0
	vmovupd	%ymm0, 32(%rdx)
.Ltmp0:
	.loc	1 6 1                           # add.c:6:1
	vzeroupper
	retq
```
* `.cfi_..., .file, .loc`などは命令ではない
* その他不要な部分を省略すると以下となる
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
  * `vmovupd (%rsi), %ymm0` : SIMDロード命令
  * `vaddpd (%rdi), %ymm0, %ymm0` : SIMD加算命令
  * `vmovupd %ymm0, 32(%rdx)` : SIMDストア命令
などが使われている
* なお, 
  * 命令の最後の2文字 `pd` は _packed double precision_ の略であり, _p_ がSIMD命令であることの証し, _d_ は64 bitの浮動小数点(`double`)が対象であることの証し
  * 命令に使われているレジスタ(`%ymm0`) は256 bitのレジスタであり, `double` であれば4つ分, もともとのコードは8要素に対する演算だったのでそれぞれが2回ずつ行われている
* 擬似的にはもともとのループを以下のように実行していることに相当する
```
z[0:4] = x[0:4] + y[0:4]; // SIMD命令
z[4:8] = x[4:8] + y[4:8]; // SIMD命令
```
* 一般にforループをベクトル化するにあたってコンパイラが試みることを擬似的に書くと, 
```
for (i = 0; i < n; i++) {
  A(i);
  B(i);
   ...
}  
```
というfor文 (`A(i), B(i)`は`i`を含んだ任意の文ということであり必ずしも関数呼び出しという意味ではない) を
```
for (i = 0; i < n; i += nl) {
  A(i:i+nl);
  B(i:i+nl);
    ...
}  
```
のように実行することである(`nl`はSIMD命令で一度に実行できる繰り返しの数で, 2, 4, 8, 16など)
* なおこの実行が正しい(SIMD命令を使わない場合と同じ結果になる)ためには, 繰り返しの実行順序を入れ替えてもよいという条件が必要で, それは並列化が合法な条件とほぼ同じだと考えれば良い
"""

""" md 

# SIMDプログラミングの方法

* ベクトル化(SIMD命令を利用する)にはいくつかの方法がある

1. **自動SIMD化:** プログラマはSIMD命令のことをあまり意識せず普通のループを書き, コンパイラがそれをSIMD命令に変換してくれることを期待する
1. OpenMP `#pragma simd`: 上記と実質的に同じだが, 実行の順番を入れ替えても良いことをプログラマが明示的にコンパイラに伝える
1. **ベクトル型拡張とintrinsics:** 複数の値をまとめたデータやそれに対する演算を明示的に扱う
1. **アセンブリ言語:**  自分でアセンブリ言語を書く

* 1.や2.で話がすむのが理想だが, 実際のところはなかなかそうはいかない. 色々工夫をしてもなかなかうまくいかない, 速くならないとイライラすることになる
* 4.は最後の手段
* 以下では多少手間はかかるものの, 努力に対する効果がある程度保証されやすい3.の方法について述べる

"""

""" md 

# ベクトル型拡張

## ベクトル型の定義

* ベクトル型は複数の要素を束ねた一つの値 --- Single Instruction Multiple Dataの "Multiple Data" に相当する値 --- を表す型である
* ユーザがそのような型を定義することもできる(多くのコンパイラでサポートされているあまり知られていない機能)
* 例えば以下で `double`を 8つ (512 bit分) まとめた一つのベクトル型 `doublev` を定義している
* なお, ベクトル型する前の型(`double`など)を区別して, `スカラ型`と呼ぶことがある

"""

""" code w """
%%writefile doublev.c
""" exec-include ./mk_version.py -D VER=doublev nb/source/cs04/include/example.c """
""" """

""" md 

## ベクトル型に対する演算

* そのようにして定義された型は`int`, `double`などと同様, 変数(関数の引数含む)や関数返り値の型として使える
* また, (ここが肝心だが), `+` や `*` などの見慣れた記法で演算が書け, それらは **ほぼ確実に** SIMD命令に変換される
* 以下は `double` 8つ分の掛け算と加算を行う関数である
* 一見して普通の `double` を対象とする関数と非常に似ていることに注意

"""

""" code w """
%%writefile doublev_fma.c
""" exec-include ./mk_version.py -D VER=doublev_fma nb/source/cs04/include/example.c """
""" """

""" code w """
%%bash
nvc -fast -Mkeepasm -Minfo -Mneginfo -c doublev_fma.c
""" """

""" code w """
%%bash
ls
""" """

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
* 今回は`%zmm`という512 bitのレジスタ --- つまり512 bitのSIMD命令 --- が使われている
* 初めに示した`add`では256 bitの命令しか使われておらず, なぜこれが使われなかったのかは不明
* 奇しくも, これがベクトル型拡張を使う理由のひとつにもなる
"""

""" md 

* また, 普通の`double`型と`doublev`型を混ぜた演算も行うことができる

"""

""" code w """
%%writefile doublev_fma_mixed.c
""" exec-include ./mk_version.py -D VER=doublev_fma_mixed nb/source/cs04/include/example.c """
""" """

""" code w """
%%bash
nvc -fast -Mkeepasm -Minfo -Mneginfo -c doublev_fma_mixed.c
""" """

""" code w """
%%bash
ls
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
* `vbroadcastsd %xmm0, %zmm0`は1つの`double`型の値(`%xmm0`)を8つ分, `%zmm0`に格納するもの
"""


""" md 

* なおベクトル型の大きさ(上記の`nl`)はCPUでサポートされているSIMD命令の幅(512 bitなど)に合わせるのが普通だが, その何倍かの値を指定することも意味がある
* その場合, ひとつのベクトル型が複数のSIMDレジスタに分けて格納される

"""

""" code w """
%%writefile doublev_fma16.c
""" exec-include ./mk_version.py -D VER=doublev_fma16 nb/source/cs04/include/example.c """
""" """

""" code w """
%%bash
nvc -fast -Mkeepasm -Minfo -Mneginfo -c doublev_fma16.c
""" """

""" code w """
%%bash
ls
""" """

""" code w """
%%bash
cat doublev_fma16.s
""" """

""" md 
* 関係するところだけを抜粋すると以下
```
doublev_fma16:                          # @doublev_fma16
	vfmadd213pd	%zmm4, %zmm2, %zmm0     # zmm0 = (zmm2 * zmm0) + zmm4
	vfmadd213pd	%zmm5, %zmm3, %zmm1     # zmm1 = (zmm3 * zmm1) + zmm5
	retq
```
"""

""" md 
## スカラ型 &lt;-&gt; ベクトル型

* スカラ型からベクトル型を作るにはいくつかの方法がある

1\. 要素を明示的に並べる文法としてはCの配列と同様の文法 `{ a, b, c, ... }` が使える

* 例: 以下は同じ要素(`u`) 8つからなるベクトル型の値を作る関数
"""

""" code w """
%%writefile uniform.c
""" exec-include ./mk_version.py -D VER=uniform nb/source/cs04/include/example.c """
""" """

""" code w """
%%bash
nvc -fast -Mkeepasm -Minfo -Mneginfo -c uniform.c
""" """

""" code w """
%%bash
ls
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
"""

""" md 

2\. ベクトル型を配列のように使う

  * ベクトル型データの各要素を`u[i] = ...`のようにセットすることができる
  * 上記のように書くと何度も同じまたは似た式を書くのが煩わしい, 要素数を変えたときに変更が必要, などの問題があるが, この方法を使えば以下のようにスッキリと書くことができる
  * ただし全体をまとめて効率的な命令にしてくれるかどうかは試してみないとわからない
"""

""" code w """
%%writefile uniform2.c
""" exec-include ./mk_version.py -D VER=uniform2 nb/source/cs04/include/example.c """
""" """

""" code w """
%%bash
nvc -fast -Mkeepasm -Minfo -Mneginfo -c uniform2.c
""" """

""" code w """
%%bash
ls
""" """

""" code w """
%%bash
cat uniform2.s
""" """

""" md 
* 関係するところを抜粋すると以下
```
uniform2:                               # @uniform2
	vbroadcastsd	%xmm0, %zmm0
	retq
```
* コンパイラがループ全体を賢く一つの命令に変換している
"""

""" md 
* 以下はこの応用で, ループのベクトル化の際によく必要な, $i$, $i+1$, $i+2$, ... からなるベクトルを作る方法
"""

""" code w """
%%writefile range.c
""" exec-include ./mk_version.py -D VER=range nb/source/cs04/include/example.c """
""" """

""" code w """
%%bash
nvc -fast -Mkeepasm -Minfo -Mneginfo -c range.c
""" """

""" code w """
%%bash
ls
""" """

""" code w """
%%bash
cat range.s
""" """

""" md 
* 以下は`double`の配列`a`の`a[i]`〜`a[i+7]`の8要素を取り出してひとつのベクトル型の値とする
"""

""" code w """
%%writefile loadv.c
""" exec-include ./mk_version.py -D VER=loadv nb/source/cs04/include/example.c """
""" """

""" code w """
%%bash
nvc -fast -Mkeepasm -Minfo -Mneginfo -c loadv.c
""" """

""" code w """
%%bash
ls
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
* 全体が一つのSIMD命令になっている
"""

""" md 
* 以下は逆にベクトル型の値を`double`の配列`a`の`a[i]`〜`a[i+7]`の8要素にセットする
"""

""" code w """
%%writefile storev.c
""" exec-include ./mk_version.py -D VER=storev nb/source/cs04/include/example.c """
""" """

""" code w """
%%bash
nvc -fast -Mkeepasm -Minfo -Mneginfo -c storev.c
""" """

""" code w """
%%bash
ls
""" """

""" code w """
%%bash
cat storev.s
""" """

""" md 
* 関係するところを抜粋すると以下
```
storev:                                 # @storev
	vmovups	%ymm0, (%rdi)
	vextractf64x4	$1, %zmm0, 32(%rdi)
	vzeroupper
	retq
```
* 512 bitの値(`%zmm0`)を半分ずつに分けて2つの命令で格納している
* なぜこうしているかは不明(詳細を知る必要はない)
"""


""" md 
* ベクトル型を用いたプログラミングの要諦は, 
  * ベクトル化したいループが与えられたときに, 適切な回数の繰り返しをまとめて実行するのが目標
  * 繰り返しごとに異なる値を持つ式を極力ベクトル型のまま計算するようにする
ということ  

"""

""" md 

# SIMDによる性能向上の目撃

* マルチコアやGPUでも用いた以下のプログラムをSIMDを用いて高速化する

"""

""" code w """
%%writefile omp_speedup_base.c
""" exec-include ./mk_version.py -D VER=base nb/source/cs04/include/omp_speedup.c """
""" """

""" code w """
%%bash
nvc -fast -Mkeepasm -Minfo -Mneginfo -o omp_speedup_base.exe omp_speedup_base.c
""" """

""" code w """
%%bash
./omp_speedup_base.exe 64 $((100 * 1000 * 1000))
""" """

""" md 

* 目標は以下のforループ
```
for (long i = 0; i < m; i++) {
  x[i] = lin_recv(0.99, i + 1, 1.0, n);
}
```
を何回か(`double`を用いているのでさしあたり 512 / 64 = 8回としておく)まとめて実行すること
* 擬似的に書けば(`nl = 8`, `m`は`nl`で割り切れると仮定して)
```
for (long i = 0; i < m; i += nl) {
  x[i:i+8] = lin_rec(0.99, i:i+8 + 1, 1.0, n);
}
```
* `i:i+8`は$i, i+1, ..., i+7`を表すインフォーマルな記法で, これまでに定義した関数を借りて書けば, 
```
for (long i = 0; i < m; i += nl) {
  storev(&x[i], lin_rec(0.99, range(i) + 1, 1.0, n));
}
```

* `lin_rec`
```
double lin_rec(double a, double b, double c, long n) {
  double t = c;
  for (long j = 0; j < n; j++) {
    t = a * t + b;
  }
  return t;
}
```
も, `b`が繰り返しごとに異なる, `t`も同様に異なることになることを反映し, `doublev`にする
```
doublev lin_rec(double a, doublev b, double c, long n) {
  doublev t = uniform(c);
  for (long j = 0; j < n; j++) {
    t = a * t + b;
  }
  return t;
}
```

* 全体をまとめると以下の通り
"""

""" code w """
%%writefile omp_speedup_simd.c
""" exec-include ./mk_version.py -D VER=simd nb/source/cs04/include/omp_speedup.c """
""" """

""" code w """
%%bash
nvc -fast -Mkeepasm -Minfo -Mneginfo -o omp_speedup_simd.exe omp_speedup_simd.c
""" """

""" code w """
%%bash
./omp_speedup_simd.exe 64 $((100 * 1000 * 1000))
""" """

""" md 

# 命令レベル並列性の向上

* 上記のコードにはさらなる性能向上の余地がある
* `nl`の値を2倍, 4倍, 8倍するとさらに性能が向上する場合がある
* 実際に`nl`の値を変えて, 最大性能が得られる値を求めてみよ
* なお, あまり本質的とは思えないが, NVIDIAコンパイラではこの`nl`として2のべき乗(1, 2, 4, 8, ...)以外の値は許さないという制限が有る(このせいで実際に最適な値に設定できない場合もある)
"""

""" code w """
%%writefile omp_speedup_simd_ilp.c
""" exec-include ./mk_version.py -D VER=simd nb/source/cs04/include/omp_speedup.c """
""" """

""" code w """
%%bash
nvc -fast -Mkeepasm -Minfo -Mneginfo -o omp_speedup_simd_ilp.exe omp_speedup_simd_ilp.c
""" """

""" code w """
%%bash
./omp_speedup_simd_ilp.exe 64 $((100 * 1000 * 1000))
""" """

""" md 
* これで性能が向上する理由を説明すると長くなるので省略するが, GPUが1つのコアの中でも複数のスレッドを実行させて性能を向上できるのと本質的な理由は同じ
* つまり, ひとつの変数だけで `t = a * t + b` をひたすら繰り返しても演算にかかる時間(遅延)のせいで, 演算器に十分なデータが供給されない
* ベクトル型の大きさを大きくする = より多くのデータに対して, `t = a * t + b` を「並行に」実行する, ことで1コアの限界性能に達する
"""


""" md 

#*P SIMD, 命令レベル並列性, マルチコアを使った性能向上

* SIMD, 命令レベル並列性(`nl`の調整), マルチコア(`#pragma omp parallel for`)すべてを組み合わせて性能を最大にせよ
* `m`や`n`を適切に調節せよ
* 得られた最大性能をGPU上の最大性能と比べてみよ
"""

""" code w """
%%writefile omp_speedup_simd_ilp_mp.c
""" exec-include ./mk_version.py -D VER=simd nb/source/cs04/include/omp_speedup.c """
""" """

""" code w """
%%bash
nvc -fast -mp=multicore -Mkeepasm -Minfo -Mneginfo -o omp_speedup_simd_ilp_mp.exe omp_speedup_simd_ilp_mp.c
""" """

""" md 
* 以下でスレッド数を変えて実行
"""

""" code w """
%%bash
OMP_PROC_BIND=true OMP_NUM_THREADS=1 ./omp_speedup_simd_ilp_mp.exe 64 $((100 * 1000 * 1000))
""" """

""" md 
* 以下を適切に修正して, 
"""

""" code w """
%%bash
for th in 1 2 3 適切なスレッド数 ; do
    echo -n "$th "
    OMP_PROC_BIND=true OMP_NUM_THREADS=${th} ./omp_speedup_simd_ilp_mp.exe $((64 * ${th})) $((100 * 1000 * 1000)) | grep GFLOPS
done
""" """

""" md 
* 結果を以下にコピペして性能向上を可視化せよ(性能向上がすぐに頭打ちになるようであれば, $m$や$n$の値を調節せよ)
"""

""" code w """
""" include nb/source/cs01/include/speedup.py """
""" """

