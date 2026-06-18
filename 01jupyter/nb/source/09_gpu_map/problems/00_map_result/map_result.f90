program map_result
  use omp_lib
  implicit none
  real :: t, a(3)
  character(len=32) :: arg
  integer :: i

  if (command_argument_count() > 0) then
     call get_command_argument(1, arg)
     read(arg, *) t
  else
     t = 10.0
  end if
  a = (/ t + 1, t + 2, t + 3 /)
  ! BEGIN ANSWER: GPUで更新した結果がホストに反映されるよう, target 構文に map(tofrom: ...) を付けよ.
  !$omp target map(tofrom: t, a)
  ! END ANSWER
  print "(a,f12.6)", "GPU: t = ", t
  print "(a,3f12.6)", "GPU: a = ", a(1), a(2), a(3)
  t = t * 2.0
  do i = 1, 3
     a(i) = a(i) * 2.0
  end do
  ! BEGIN ANSWER: 上で始めた target 領域を閉じる (!$omp end target).
  !$omp end target
  ! END ANSWER
  print "(a,f12.6)", "CPU: t = ", t
  print "(a,3f12.6)", "CPU: a = ", a(1), a(2), a(3)
end program map_result
