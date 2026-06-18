program omp_for
  use omp_lib
  integer :: m, n, i, j, nthreads, stat
  character(len=32) :: arg, env
  nthreads = 1
  call get_environment_variable("OMP_NUM_THREADS", env, status=stat)
  if (stat == 0) read (env, *) nthreads
  if (mod(nthreads, 32) /= 0) then
     write (0, "(a,i0,a)") "OMP_NUM_THREADS (", nthreads, ") must be a multiple of 32"
     stop 1
  end if
  m = 5
  n = 6
  if (command_argument_count() > 0) then
     call get_command_argument(1, arg)
     read (arg, *) m
  end if
  if (command_argument_count() > 1) then
     call get_command_argument(2, arg)
     read (arg, *) n
  end if
  print "(a)", "hello on host"
  !$omp target teams
  print "(a,i3.3,a,i3.3)", "in teams: ", omp_get_team_num(), "/", omp_get_num_teams()
  !$omp distribute private(i, j)
  do i = 0, m - 1
     print "(a,i3.3,a,i3.3,a,i3.3)", "in distribute: i=", i, &
          " executed by ", omp_get_team_num(), "/", omp_get_num_teams()
     !$omp parallel num_threads(nthreads) private(j)
     print "(a,i3.3,a,i3.3,a,i3.3,a,i3.3,a,i3.3)", "in parallel: i=", i, " ", &
          omp_get_team_num(), "/", omp_get_num_teams(), " ", &
          omp_get_thread_num(), "/", omp_get_num_threads()
     !$omp do
     do j = 0, n - 1
        print "(a,i3.3,a,i3.3,a,i3.3,a,i3.3,a,i3.3,a,i3.3)", "in for: i=", i, " j=", j, &
             " ", omp_get_team_num(), "/", omp_get_num_teams(), " ", &
             omp_get_thread_num(), "/", omp_get_num_threads()
     end do
     !$omp end do
     !$omp end parallel
  end do
  !$omp end distribute
  !$omp end teams
  print "(a)", "back on host"
end program omp_for
