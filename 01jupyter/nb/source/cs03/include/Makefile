c_src:=$(wildcard omp_ball.c)
base_c:=$(patsubst %.c,%_base.c,$(c_src))
mp_c:=$(patsubst %.c,%_mp.c,$(c_src))
gpu_c:=$(patsubst %.c,%_gpu.c,$(c_src))

base_targets:=$(patsubst %.c,%_base.exe,$(c_src))
mp_targets:=$(patsubst %.c,%_mp.exe,$(c_src))
gpu_targets:=$(patsubst %.c,%_gpu.exe,$(c_src))

all : $(gpu_targets) $(mp_targets) $(base_targets) 

# -Mkeepasm -Mvect -Minfo -Mneginfo
# -Minfo -Mneginfo -Mkeepasm -mavx512f -mfma
# -gpu=mem:managed 

$(base_targets) : %_base.exe : %_base.c
	nvc -fast $< -o $@

$(mp_targets) : %_mp.exe : %_mp.c
	nvc -fast -mp=multicore $< -o $@

$(gpu_targets) : %_gpu.exe : %_gpu.c
	nvc -fast -mp=gpu $< -o $@

$(base_c) : %_base.c : %.c
	./mk_version.py -D VER=base $< > $@

$(mp_c) : %_mp.c : %.c
	./mk_version.py -D VER=mp $< > $@

$(gpu_c) : %_gpu.c : %.c
	./mk_version.py -D VER=gpu $< > $@

clean :
	rm -f *.exe
	mv -f *_base.c *_gpu.c *_mp.c trash/

.DELETE_ON_ERROR:
