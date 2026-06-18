program omp_map_tofrom
  use omp_lib
  implicit none
  type point
     real :: x
     real :: y
  end type point
  real :: t, a(3)
  type(point) :: p
  character(len=32) :: arg
  integer :: i

  if (command_argument_count() > 0) then
     call get_command_argument(1, arg)
     read(arg, *) t
  else
     t = 10.0
  end if
  a = (/ t, t + 1, t + 2 /)
  p = point(t + 3, t + 4)
  ! map(tofrom: ...) を指定すると, GPUでの書き換えがCPUに戻ってくる
  ! Fortranでは配列全体は a, 一部は a(1:n) のように指定する
  !$omp target map(tofrom: t, a, p)
  print "(a,f12.6)", "GPU: t = ", t
  print "(a,3f12.6)", "GPU: a = ", a(1), a(2), a(3)
  print "(a,2f12.6)", "GPU: p = ", p%x, p%y
  t = t * 2.0
  do i = 1, 3
     a(i) = a(i) * 2.0
  end do
  p%x = p%x * 2.0
  p%y = p%y * 2.0
  !$omp end target
  print "(a,f12.6)", "CPU: t = ", t
  print "(a,3f12.6)", "CPU: a = ", a(1), a(2), a(3)
  print "(a,2f12.6)", "CPU: p = ", p%x, p%y
end program omp_map_tofrom
