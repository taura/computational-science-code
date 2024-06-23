c_src:=$(wildcard example.c)
bases:=$(patsubst %.c,%,$(c_src))
vers:=add doublev doublev_fma doublev_fma_mixed doublev_fma16 uniform uniform2 linear loadv storev
vers_c:=$(foreach base,$(bases),$(foreach ver,$(vers),vers/$(base)_$(ver).c))
vers_s:=$(patsubst %.c,%.s,$(vers_c))

all : $(vers_s)

# -Mkeepasm -Mvect -Minfo -Mneginfo
# -Minfo -Mneginfo -Mkeepasm -mavx512f -mfma
# -gpu=mem:managed

define rule
vers/$(base)_$(ver).c : $(base).c vers/dir
	./mk_version.py -D VER=$(ver) $$< > $$@
endef

$(vers_s) : %.s : %.c
	nvc -tp=native -fast -Mkeepasm -Minfo -Mneginfo $< -c -o $*.o

vers/dir :
	mkdir -p $@

vers_c:=$(foreach base,$(bases),$(foreach ver,$(vers),$(eval $(call rule))))


clean :
	rm -rf vers

.DELETE_ON_ERROR:
