module integral_mod
contains
  function int_inv_1_x2(a, b, n) result(s)
    real(8), intent(in) :: a, b
    integer(8), intent(in) :: n
    real(8) :: s, dx, x
    integer(8) :: i
    s = 0.0d0
    dx = (b - a) / real(n, 8)
    ! TODO: 下のループを reduction(+:s) を用いて並列化し, 総和の競合を解消せよ.
    ! BEGIN ANSWER
    !$omp parallel do private(x) reduction(+:s)
    ! END ANSWER
    do i = 0, n - 1
       x = a + i * dx
       s = s + 1 / (1 + x * x)
    end do
    ! BEGIN ANSWER
    !$omp end parallel do
    ! END ANSWER
    s = s * dx
  end function int_inv_1_x2
end module integral_mod

program reduction_sum
  use integral_mod
  character(len=64) :: arg
  real(8) :: a, b, s
  integer(8) :: n
  a = 0.0d0; b = 1.0d0; n = 4000_8 * 1000_8 * 1000_8
  if (command_argument_count() >= 1) then
     call get_command_argument(1, arg); read (arg, *) a
  end if
  if (command_argument_count() >= 2) then
     call get_command_argument(2, arg); read (arg, *) b
  end if
  if (command_argument_count() >= 3) then
     call get_command_argument(3, arg); read (arg, *) n
  end if
  s = int_inv_1_x2(a, b, n)
  print "(a,f0.6)", "s = ", s
end program reduction_sum
