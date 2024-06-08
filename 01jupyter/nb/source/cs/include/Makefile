c_src:=omp_target.c

mp_targets:=$(patsubst %.c,%_mp.exe,$(c_src))
gpu_targets:=$(patsubst %.c,%_gpu.exe,$(c_src))

all : $(mp_targets) $(gpu_targets)

$(mp_targets) : %_mp.exe : %.c
	nvc -o $@ -mp=multicore $<

$(gpu_targets) : %_gpu.exe : %.c
	nvc -o $@ -mp=gpu $<

clean :
	rm -f *.exe
