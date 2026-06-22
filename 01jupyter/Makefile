
in_dir := nb/source
out_dir := notebooks/source
langs := cpp f90
nn_topics := $(patsubst $(in_dir)/%,%,$(wildcard $(in_dir)/[0-9][0-9]_*))

# GPU トピックは -mp=gpu, それ以外は -mp=multicore でコンパイルする
# (入門など OpenMP を使わない問題でも -mp=multicore は無害)
gpu_topics := 07_gpu_target 08_gpu_teams 09_gpu_map 10_gpu_speedup
mpflag = $(if $(filter $(nn_topic),$(gpu_topics)),-mp=gpu,-mp=multicore)

deploy :

# 01_intro -> intro
define topic_of
$(shell echo $(1) | sed 's|[0-9]*_||')
endef

# 00_average -> average
define problem_of
$(shell echo $(1) | sed 's|[0-9]*_||')
endef

# $(call dir_in,D) -> directories in D
define dir_in
$(shell find $(1) -mindepth 1 -maxdepth 1 -type d -printf "%f\n" 2>/dev/null)
endef

# $(call ext_in,D,e) -> files of extension e in D
define ext_in
$(shell test -d $(1) && find $(1) -mindepth 1 -maxdepth 1 -name '*.$(2)' -exec basename -s .$(2) {} +)
endef

# deploy .ipynb from the topic .py file (e.g., parallel.py)
# e.g.,
# nb/source/01_intro/intro.py -> notebooks/source/01_intro/intro.ipynb
define deploy_topic
deploy : $(out_dir)/$(nn_topic)/$(topic).ipynb
$(out_dir)/$(nn_topic)/$(topic).ipynb : $(out_dir)/$(nn_topic)/dir
# topic .py は {var} 置換を使わない (本文に素の波括弧を含む) ので mk_nb に直接渡す
$(out_dir)/$(nn_topic)/$(topic).ipynb :  $(in_dir)/$(nn_topic)/$(topic).py
	./mk_nb.py --syntax wisteria $$< > $$@

$(out_dir)/$(nn_topic)/dir :
	mkdir -p $$@
endef

# deploy .ipynb from a problem .py file as well as it problem.md
# e.g.,
# nb/source/01_intro/problems/00_average/average.py -> notebooks/source/01_intro/problems/00_average/average.ipynb
# nb/source/01_intro/problems/00_average/problem.md -> notebooks/source/01_intro/problems/00_average/problem.md

define deploy_problem
# .py -> .ipynb
$(out_dir)/$(nn_topic)/$(topic).ipynb : $(out_dir)/$(nn_topic)/problems/$(nn_problem)/$(problem).ipynb
$(out_dir)/$(nn_topic)/problems/$(nn_problem)/$(problem).ipynb : $(out_dir)/$(nn_topic)/problems/$(nn_problem)/problem.md
# 問題ごとに setup.py (任意) があれば, その中身をテンプレの {setup} に流し込む
# (データ/重みの準備セルなど。無ければ {setup} は空になる)。setup.py は .py 形式なのでセルになる。
$(out_dir)/$(nn_topic)/problems/$(nn_problem)/$(problem).ipynb :  $(wildcard $(in_dir)/$(nn_topic)/problems/$(nn_problem)/setup.py)
$(out_dir)/$(nn_topic)/problems/$(nn_problem)/$(problem).ipynb :  $(in_dir)/$(nn_topic)/problems/$(nn_problem)/$(problem).py
	./format_string.py in_dir=$(in_dir) out_dir=$(out_dir) nn_topic=$(nn_topic) topic=$(topic) nn_problem=$(nn_problem) problem=$(problem) mpflag=$(mpflag) setup="$$$$(cat $(in_dir)/$(nn_topic)/problems/$(nn_problem)/setup.py 2>/dev/null)" $$< | ./mk_nb.py --syntax wisteria - > $$@

# problem.md -> copy (内容なのでそのままコピー. コード中の波括弧を format しないこと)
$(out_dir)/$(nn_topic)/problems/$(nn_problem)/problem.md : $(out_dir)/$(nn_topic)/problems/$(nn_problem)/dir
$(out_dir)/$(nn_topic)/problems/$(nn_problem)/problem.md :  $(in_dir)/$(nn_topic)/problems/$(nn_problem)/problem.md
	cp $$< $$@

# mkdir
$(out_dir)/$(nn_topic)/problems/$(nn_problem)/dir : 
	mkdir -p $$@
endef

# deploy .cpp/.f90 files (学生向けの穴あきコード)
# 答え部分を隠した穴あきコードを in_dir 側の to_students/ に書き出す.
# これは .ipynb に %%writefile_ で埋め込まれる中身であり, 出力 (out_dir) には
# 単体ファイルとしては置かない (.ipynb に埋め込み済みなので冗長なため).
# e.g.,
# nb/source/01_intro/problems/00_average/average.{cpp,f90}
#   -> nb/source/01_intro/problems/00_average/to_students/average.{cpp,f90}
define deploy_code
$(out_dir)/$(nn_topic)/problems/$(nn_problem)/$(problem).ipynb : $(in_dir)/$(nn_topic)/problems/$(nn_problem)/to_students/$(problem).$(lang)
$(in_dir)/$(nn_topic)/problems/$(nn_problem)/to_students/$(problem).$(lang) :  $(in_dir)/$(nn_topic)/problems/$(nn_problem)/$(problem).$(lang) strip_answer.py
	mkdir -p $(in_dir)/$(nn_topic)/problems/$(nn_problem)/to_students && ./strip_answer.py $$< > $$@
endef

# 問題ディレクトリに data/ があれば (MNIST など) 出力側へまるごとコピーする
define deploy_data
$(out_dir)/$(nn_topic)/problems/$(nn_problem)/$(problem).ipynb : $(out_dir)/$(nn_topic)/problems/$(nn_problem)/data
$(out_dir)/$(nn_topic)/problems/$(nn_problem)/data : $(wildcard $(in_dir)/$(nn_topic)/problems/$(nn_problem)/data/*)
	mkdir -p $$@ && cp -r $(in_dir)/$(nn_topic)/problems/$(nn_problem)/data/. $$@/
endef

EVAL=$(eval $(1))
#EVAL=$(info $(1))

$(foreach nn_topic,$(nn_topics),\
  $(foreach topic,$(call topic_of,$(nn_topic)),\
    $(call EVAL,$(call deploy_topic)) \
    $(foreach nn_problem,$(call dir_in,$(in_dir)/$(nn_topic)/problems),\
      $(foreach problem,$(call problem_of,$(nn_problem)),\
        $(call EVAL,$(call deploy_problem)) \
        $(if $(wildcard $(in_dir)/$(nn_topic)/problems/$(nn_problem)/data),$(if $(wildcard $(in_dir)/$(nn_topic)/problems/$(nn_problem)/setup.py),,$(call EVAL,$(call deploy_data)))) \
        $(foreach lang,$(langs),\
	  $(call EVAL,$(call deploy_code)) \
)))))

.SHELLFLAGS := -eu -o pipefail -c
.DELETE_ON_ERROR:
