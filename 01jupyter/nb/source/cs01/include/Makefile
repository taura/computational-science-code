c_src:=$(wildcard *.c)

mp_targets:=$(patsubst %.c,%_mp.exe,$(c_src))
gpu_targets:=$(patsubst %.c,%_gpu.exe,$(c_src))
CFLAGS:=-Wall -Wextra -O3 -mfma -fopenmp 

all : $(mp_targets) # $(gpu_targets)

# 
$(mp_targets) : %_mp.exe : %.c
	gcc $(CFLAGS) -o $@ $<
#	nvc -fast -mp=multicore $< -o $@

$(gpu_targets) : %_gpu.exe : %.c
	nvc -o $@ -mp=gpu $<

clean :
	rm -f *.exe
