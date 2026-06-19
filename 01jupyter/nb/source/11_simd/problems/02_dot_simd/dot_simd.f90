! 内積 s = Σ x(i)*y(i) を n 要素について計算する.
! 注: Fortran では組込み関数 dot_product(x, y) も同様にSIMD化される.
function dot(n, x, y) result(s)
  implicit none
  integer(8), intent(in) :: n
  real(8), intent(in) :: x(n), y(n)
  real(8) :: s
  integer(8) :: i
  s = 0.0d0
  ! BEGIN ANSWER: 内積の総和ループを simd reduction でSIMD化せよ (下の do の直前に1行追加).
  !$omp simd reduction(+:s)
  ! END ANSWER
  do i = 1, n
     s = s + x(i) * y(i)
  end do
end function dot

program dot_simd
  implicit none
  character(len=32) :: arg
  integer(8) :: n, i
  real(8), allocatable :: x(:), y(:)
  real(8) :: s, expected, dot
  n = 100_8 * 1000 * 1000
  if (command_argument_count() >= 1) then
     call get_command_argument(1, arg); read (arg, *) n
  end if
  allocate(x(n), y(n))
  do i = 1, n
     x(i) = 1.0d0; y(i) = 2.0d0
  end do

  s = dot(n, x, y)

  ! x(i)=1, y(i)=2 なので理論値は 2*n
  expected = 2.0d0 * real(n, 8)
  if (s == expected) then
     print "(a,f0.1,a)", "OK: s=", s, " (= 2*n)"
  else
     print "(a,f0.1,a,f0.1)", "NG: s=", s, ", expected=", expected
  end if
end program dot_simd
