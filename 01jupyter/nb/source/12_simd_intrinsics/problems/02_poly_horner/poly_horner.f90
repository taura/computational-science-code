! 注: ベクトル型 (vector_size) は C/C++ 独自の拡張で Fortran には無い.
! Fortran では普通のループで Horner 法を書けば, コンパイラが自動的にSIMD化する.
! 多項式 p(x) = c(1) + c(2)*x + c(3)*x^2 + c(4)*x^3
program poly_horner
  implicit none
  character(len=32) :: arg
  integer(8) :: n, i, err
  integer :: k
  integer, parameter :: deg = 3
  real(8) :: c(0:deg) = (/ 1.0d0, 2.0d0, 3.0d0, 4.0d0 /)
  real(8) :: acc, r, d
  real(8), allocatable :: x(:), p(:)
  n = 64
  if (command_argument_count() >= 1) then
     call get_command_argument(1, arg); read (arg, *) n
  end if
  allocate(x(n), p(n))
  do i = 1, n
     x(i) = 0.001d0 * real(i - 1, 8)
  end do

  ! 各要素について Horner 法 acc = acc*x + c_k で多項式を評価する
  do i = 1, n
     ! TODO: Horner法 acc = acc*x + c_k で p(i) を求めよ.
     ! BEGIN ANSWER
     acc = c(deg)
     do k = deg - 1, 0, -1
        acc = acc * x(i) + c(k)
     end do
     p(i) = acc
     ! END ANSWER
  end do

  err = 0
  do i = 1, n
     r = c(deg)
     do k = deg - 1, 0, -1
        r = r * x(i) + c(k)
     end do
     d = p(i) - r
     if (d < -1.0d-9 .or. d > 1.0d-9) err = err + 1
  end do
  if (err == 0) then
     print "(a,f0.3,a,i0,a,f0.3)", "OK: p(1)=", p(1), ", p(", n, ")=", p(n)
  else
     print "(a,i0,a)", "NG: ", err, " 要素が不正"
  end if
end program poly_horner
