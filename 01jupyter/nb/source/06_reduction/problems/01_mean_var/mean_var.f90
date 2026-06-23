program mean_var
  integer(8), parameter :: n = 400000000_8
  real(8) :: s, sq, x, mean, var
  integer(8) :: i
  s = 0.0d0
  sq = 0.0d0
  ! TODO: 下のループを !$omp parallel do private(x) reduction(+:s,sq) で並列化し, 2つの総和の競合を解消せよ.
  ! BEGIN ANSWER
  !$omp parallel do private(x) reduction(+:s,sq)
  ! END ANSWER
  do i = 0, n - 1
     x = sin(real(mod(i, 1000_8), 8))   ! データを配列に置かず逐次生成 (並列化の本質は2つの総和)
     s  = s + x
     sq = sq + x * x
  end do
  ! BEGIN ANSWER
  !$omp end parallel do
  ! END ANSWER
  mean = s / n
  var  = sq / n - mean * mean
  print "(a,f0.6,a,f0.6)", "mean = ", mean, ", variance = ", var
end program mean_var
