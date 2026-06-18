! y(i) = a * x(i) + y(i) (saxpy/axpy) を n 要素について行う
subroutine saxpy(n, a, x, y)
  implicit none
  integer(8), intent(in) :: n
  real(8), intent(in) :: a
  real(8), intent(in) :: x(n)
  real(8), intent(inout) :: y(n)
  integer(8) :: i
  ! TODO: 下の do ループの直前に !$omp simd を1行追加し, このループをSIMD化せよ.
  do i = 1, n
     y(i) = a * x(i) + y(i)
  end do
end subroutine saxpy
