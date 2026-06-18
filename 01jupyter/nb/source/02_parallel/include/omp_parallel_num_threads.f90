program omp_parallel_num_threads
  character(len=32) :: arg
  integer :: nt
  call get_command_argument(1, arg)
  read (arg, *) nt
  print "(a)", "before parallel"
  !$omp parallel num_threads(nt)
  print "(a)", "in parallel"
  !$omp end parallel
  print "(a)", "after parallel"
end program omp_parallel_num_threads
