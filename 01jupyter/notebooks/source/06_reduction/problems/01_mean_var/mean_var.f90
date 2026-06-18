program mean_var
  integer(8), parameter :: n = 1000000_8
  real(8), allocatable :: a(:)
  real(8) :: s, sq, x, mean, var
  integer(8) :: i
  allocate(a(0:n-1))
  do i = 0, n - 1
     a(i) = sin(real(i, 8))
  end do
  s = 0.0d0
  sq = 0.0d0
  ! TODO: 下のループを !$omp parallel do private(x) reduction(+:s,sq) で並列化し, 2つの総和の競合を解消せよ.
  do i = 0, n - 1
     x = a(i)
     s  = s + x
     sq = sq + x * x
  end do
  ! TODO: 上で始めた parallel do 領域を閉じる (!$omp end parallel do).
  mean = s / n
  var  = sq / n - mean * mean
  print "(a,f0.6,a,f0.6)", "mean = ", mean, ", variance = ", var
end program mean_var
