c_src:=$(wildcard omp_target.c omp_teams.c omp_target_teams.c omp_team_num.c omp_distribute.c omp_target_teams_distribute.c omp_parallel.c omp_distribute_parallel.c omp_for.c omp_map_local.c omp_map_global.c omp_speedup.c)

mp_targets:=$(patsubst %.c,%_mp.exe,$(c_src))
gpu_targets:=$(patsubst %.c,%_gpu.exe,$(c_src))
CFLAGS:=-Wall -Wextra -O3 -mfma -fopenmp 

all : $(gpu_targets) $(mp_targets)

# 
$(mp_targets) : %_mp.exe : %.c
#	gcc $(CFLAGS) -o $@ $<
	nvc -fast -mp=multicore $< -o $@

$(gpu_targets) : %_gpu.exe : %.c
	nvc -fast -mp=gpu -gpu=mem:managed $< -o $@

clean :
	rm -f *.exe
