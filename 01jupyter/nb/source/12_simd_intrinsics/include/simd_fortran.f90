! Fortran には C/C++ の vector_size のようなベクトル型拡張は無い.
! 明示的に SIMD 化を促すには !$omp simd 指示行や配列演算を用いる.

subroutine axpy(n, a, x, y, z)
  implicit none
  integer, intent(in) :: n
  real(8), intent(in) :: a
  real(8), intent(in) :: x(n), y(n)
  real(8), intent(out) :: z(n)
  integer :: i

  ! 方法1: !$omp simd でループを SIMD 化する
  !$omp simd
  do i = 1, n
     z(i) = a * x(i) + y(i)
  end do
  !$omp end simd
end subroutine axpy

subroutine axpy_array(n, a, x, y, z)
  implicit none
  integer, intent(in) :: n
  real(8), intent(in) :: a
  real(8), intent(in) :: x(n), y(n)
  real(8), intent(out) :: z(n)

  ! 方法2: 配列まるごとの式(配列構文). コンパイラの自動ベクトル化に委ねる
  z = a * x + y
end subroutine axpy_array
