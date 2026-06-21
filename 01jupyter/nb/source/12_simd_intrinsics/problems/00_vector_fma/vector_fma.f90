! Fortran には C/C++ のベクトル型拡張 (vector_size) に相当する機能が無い.
! そこで, 同じ y(i) = a(i) * b(i) + c(i) の積和 (fma) をループで書き,
! !$omp simd で SIMD 化する.
subroutine vector_fma(n, a, b, c, y)
  implicit none
  integer(8), intent(in) :: n
  real(8), intent(in) :: a(n), b(n), c(n)
  real(8), intent(out) :: y(n)
  integer(8) :: i
  ! TODO: 下の do ループの直前に !$omp simd を1行追加し, このループをSIMD化せよ.
  ! BEGIN ANSWER
  !$omp simd
  ! END ANSWER
  do i = 1, n
     y(i) = a(i) * b(i) + c(i)
  end do
end subroutine vector_fma
