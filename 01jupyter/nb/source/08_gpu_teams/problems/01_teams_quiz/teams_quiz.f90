program teams_quiz
  use omp_lib
  implicit none
  character(len=32) :: arg
  integer :: nthreads, m, n, i, j, stat

  nthreads = 1
  call get_environment_variable("OMP_NUM_THREADS", arg, status=stat)
  if (stat == 0) read (arg, *) nthreads
  if (nthreads /= 1 .and. mod(nthreads, 32) /= 0) then
     write (0, "(a,i0,a)") "OMP_NUM_THREADS (", nthreads, &
          ") must be 1 or a multiple of 32"
     stop 1
  end if

  if (command_argument_count() >= 1) then
     call get_command_argument(1, arg); read (arg, *) m
  else
     m = 5
  end if
  if (command_argument_count() >= 2) then
     call get_command_argument(2, arg); read (arg, *) n
  else
     n = 6
  end if

  !$omp target teams
  print "(a,i3.3,a,i3.3)", "in teams: team ", &
       omp_get_team_num(), "/", omp_get_num_teams()
  !$omp distribute
  do i = 0, m - 1
     print "(a,i3.3,a,i3.3,a,i3.3)", "in distribute: i=", i, &
          " team ", omp_get_team_num(), "/", omp_get_num_teams()
     !$omp parallel num_threads(nthreads)
     print "(a,i3.3,a,i3.3,a,i3.3,a,i3.3,a,i3.3)", "in parallel: i=", i, &
          " team ", omp_get_team_num(), "/", omp_get_num_teams(), &
          " thread ", omp_get_thread_num(), "/", omp_get_num_threads()
     !$omp end parallel
     !$omp do
     do j = 0, n - 1
        print "(a,i3.3,a,i3.3,a,i3.3,a,i3.3,a,i3.3,a,i3.3)", "in for: i=", i, &
             " j=", j, " team ", omp_get_team_num(), "/", omp_get_num_teams(), &
             " thread ", omp_get_thread_num(), "/", omp_get_num_threads()
     end do
     !$omp end do
  end do
  !$omp end distribute
  !$omp end target teams
end program teams_quiz
