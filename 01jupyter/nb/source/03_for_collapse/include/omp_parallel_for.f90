program omp_parallel_for
  use omp_lib
  character(len=32) :: arg
  integer :: m, i
  if (command_argument_count() >= 1) then
     call get_command_argument(1, arg)
     read (arg, *) m
  else
     m = 10
  end if
  print "(a)", "before parallel"
  !$omp parallel
  print "(a)", "in parallel, before do"
  !$omp do
  do i = 0, m - 1
     print "(a,i3)", "i = ", i
  end do
  !$omp end do
  print "(a)", "in parallel, after do"
  !$omp end parallel
  print "(a)", "after parallel"
end program omp_parallel_for
