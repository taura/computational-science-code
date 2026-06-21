program offload_loop
  use omp_lib
  implicit none
  character(len=32) :: arg
  integer(8) :: m, i

  if (command_argument_count() >= 1) then
     call get_command_argument(1, arg); read (arg, *) m
  else
     m = 8
  end if
  ! TODO: 下の do ループを !$omp target teams distribute parallel do ... !$omp end target teams distribute parallel do で囲み, ループをGPU上の多数のチーム×スレッドで並列実行させよ. (結果を表示するだけなので map 節は不要)
  ! BEGIN ANSWER
  !$omp target teams distribute parallel do
  ! END ANSWER
  do i = 1, m
     print "(a,i0,a,i0,a,i0)", "i = ", i, "  executed by team ", &
          omp_get_team_num(), "  thread ", omp_get_thread_num()
  end do
  ! BEGIN ANSWER
  !$omp end target teams distribute parallel do
  ! END ANSWER
end program offload_loop
