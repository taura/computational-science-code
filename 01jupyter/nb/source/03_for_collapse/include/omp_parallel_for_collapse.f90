program omp_parallel_for_collapse
  use omp_lib
  use iso_c_binding
  interface
     subroutine usleep(usec) bind(c, name="usleep")
       import :: c_int
       integer(c_int), value :: usec
     end subroutine usleep
  end interface
  character(len=32) :: arg
  integer :: m, i, j, nt, t
  if (command_argument_count() >= 1) then
     call get_command_argument(1, arg)
     read (arg, *) m
  else
     m = 4
  end if
  !$omp parallel private(i, j, t)
  nt = omp_get_num_threads()
  !$omp do collapse(2)
  do i = 0, m - 1
     do j = 0, m - 1
        t = omp_get_thread_num()
        call usleep(int((i + j) * 100000, c_int))
        print "(a,i3,a,i3,a,i3,a,i3)", "i,j = ", i, ",", j, &
             ", by thread ", t, " of ", nt
     end do
  end do
  !$omp end do
  !$omp end parallel
end program omp_parallel_for_collapse
