program omp_team_num
  use omp_lib
  print "(a)", "hello on host"
  !$omp target
  !$omp teams
  print "(a,i3.3,a,i3.3)", "in teams: ", omp_get_team_num(), "/", omp_get_num_teams()
  !$omp end teams
  !$omp end target
  print "(a)", "back on host"
end program omp_team_num
