
in_dir := nb/source
out_dir := notebooks/source
langs := cpp f90
nn_topics := $(patsubst $(in_dir)/%,%,$(wildcard $(in_dir)/01_*))

deploy :

define topic_of
$(shell echo $(1) | sed 's|[0-9]*_||')
endef

define problem_of
$(shell echo $(1) | sed 's|[0-9]*_||')
endef

# $(call dir_in,D) -> directories in D
define dir_in
$(shell find $(1) -mindepth 1 -maxdepth 1 -type d -printf "%f\n")
endef

# $(call ext_in,D,e) -> .e files in D
define ext_in
$(shell test -d $(1) && find $(1) -mindepth 1 -maxdepth 1 -name '*.$(2)' -exec basename -s .$(2) {} +)
endef

# deploy .ipynb from the topic .py file (e.g., parallel.py)
define deploy_topic
deploy : $(out_dir)/$(nn_topic)/$(topic).ipynb
$(out_dir)/$(nn_topic)/$(topic).ipynb : $(out_dir)/$(nn_topic)/dir
$(out_dir)/$(nn_topic)/$(topic).ipynb :  $(in_dir)/$(nn_topic)/$(topic).py
	./format_string.py in_dir=$(in_dir) out_dir=$(out_dir) nn_topic=$(nn_topic) topic=$(topic) $$< | ./mk_nb.py --syntax wisteria - > $$@

$(out_dir)/$(nn_topic)/dir :
	mkdir -p $$@
endef

# deploy .ipynb from a problem .py file as well as it problem.md
define deploy_problem
# .py -> .ipynb
$(out_dir)/$(nn_topic)/$(topic).ipynb : $(out_dir)/$(nn_topic)/problems/$(nn_problem)/$(problem).ipynb
$(out_dir)/$(nn_topic)/problems/$(nn_problem)/$(problem).ipynb : $(out_dir)/$(nn_topic)/problems/$(nn_problem)/problem.md
$(out_dir)/$(nn_topic)/problems/$(nn_problem)/$(problem).ipynb :  $(in_dir)/$(nn_topic)/problems/$(nn_problem)/$(problem).py
	./format_string.py in_dir=$(in_dir) out_dir=$(out_dir) nn_topic=$(nn_topic) topic=$(topic) nn_problem=$(nn_problem) problem=$(problem) $$< | ./mk_nb.py --syntax wisteria - > $$@

# problem.md -> copy
$(out_dir)/$(nn_topic)/problems/$(nn_problem)/problem.md : $(out_dir)/$(nn_topic)/problems/$(nn_problem)/dir
$(out_dir)/$(nn_topic)/problems/$(nn_problem)/problem.md :  $(in_dir)/$(nn_topic)/problems/$(nn_problem)/problem.md
	./format_string.py in_dir=$(in_dir) out_dir=$(out_dir) nn_topic=$(nn_topic) topic=$(topic) nn_problem=$(nn_problem) problem=$(problem) $$< > $$@

# mkdir
$(out_dir)/$(nn_topic)/problems/$(nn_problem)/dir : 
	mkdir -p $$@
endef

define deploy_code
$(out_dir)/$(nn_topic)/problems/$(nn_problem)/$(problem).ipynb : $(out_dir)/$(nn_topic)/problems/$(nn_problem)/$(problem).$(lang)
$(out_dir)/$(nn_topic)/problems/$(nn_problem)/$(problem).$(lang) :  $(in_dir)/$(nn_topic)/problems/$(nn_problem)/$(problem).$(lang)
	cp $$< $$@
endef

EVAL=$(eval $(1))
#EVAL=$(info $(1))

$(foreach nn_topic,$(nn_topics),\
  $(foreach topic,$(call topic_of,$(nn_topic)),\
    $(call EVAL,$(call deploy_topic)) \
    $(foreach nn_problem,$(call dir_in,$(in_dir)/$(nn_topic)/problems),\
      $(foreach problem,$(call problem_of,$(nn_problem)),\
        $(call EVAL,$(call deploy_problem)) \
        $(foreach lang,$(langs),\
	  $(call EVAL,$(call deploy_code)) \
)))))

.SHELLFLAGS := -eu -o pipefail -c
.DELETE_ON_ERROR:
