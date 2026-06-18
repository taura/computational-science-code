subroutine saxpy(n, a, x, y)
  integer :: n
  real(8) :: a, x(n), y(n)
  integer :: i
  !$omp simd
  do i = 1, n
     y(i) = a * x(i) + y(i)
  end do
end subroutine saxpy
