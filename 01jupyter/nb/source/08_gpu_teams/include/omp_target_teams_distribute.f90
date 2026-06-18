program omp_target_teams_distribute
  use omp_lib
  integer :: m, i
  character(len=32) :: arg
  m = 5
  if (command_argument_count() > 0) then
     call get_command_argument(1, arg)
     read (arg, *) m
  end if
  print "(a)", "hello on host"
  !$omp target teams distribute private(i)
  do i = 0, m - 1
     print "(a,i3.3,a,i3.3,a,i3.3)", "in distribute: i=", i, &
          " executed by ", omp_get_team_num(), "/", omp_get_num_teams()
  end do
  !$omp end target teams distribute
  print "(a)", "back on host"
end program omp_target_teams_distribute
