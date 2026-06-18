program parallel_for
  use omp_lib
  integer, parameter :: n = 8
  integer :: a(0:n-1)
  integer :: i
  ! TODO: 下の do ループを !$omp parallel do ... !$omp end parallel do で囲み, 繰り返しを複数のスレッドに分担させよ.
  do i = 0, n - 1
     a(i) = i * i
     print "(a,i0,a,i0,a,i0,a)", "a[", i, "] = ", a(i), &
          char(9)//"(thread ", omp_get_thread_num(), ")"
  end do
  ! TODO: 上で始めた parallel do 領域を閉じる (!$omp end parallel do).
end program parallel_for
