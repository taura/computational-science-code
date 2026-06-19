! saxpy: y(i) = a*x(i) + y(i).
! Fortran では配列引数 x, y は「重ならない (エイリアスしない)」と仮定してよいので,
! コンパイラは何も指示しなくても素直に自動ベクトル化できる (versioning も不要).
subroutine saxpy(n, a, x, y)
  integer(8), intent(in) :: n
  real(8), intent(in) :: a, x(n)
  real(8), intent(inout) :: y(n)
  integer(8) :: i
  do i = 1, n
     y(i) = a * x(i) + y(i)
  end do
end subroutine saxpy

program autovec
  character(len=32) :: arg
  integer(8) :: n, i
  real(8), allocatable :: x(:), y(:)
  n = 16
  if (command_argument_count() >= 1) then
     call get_command_argument(1, arg); read (arg, *) n
  end if
  allocate(x(n), y(n))
  do i = 1, n
     x(i) = i - 1; y(i) = 0.0d0
  end do
  call saxpy(n, 2.0d0, x, y)
  print "(a,f0.1,a,i0,a,f0.1)", "y(1) = ", y(1), ", y(", n, ") = ", y(n)
end program autovec
