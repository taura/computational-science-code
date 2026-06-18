program manual_partition
  use omp_lib
  integer, parameter :: n = 100
  real(8) :: a(0:n-1), s
  integer :: i, tid, nt, lo, hi
  do i = 0, n - 1
     a(i) = i + 1
  end do
  ! TODO: 下のブロックを !$omp parallel private(tid, nt, lo, hi, i, s) ... !$omp end parallel で囲み, 各スレッドが自分の担当範囲を計算するようにせよ.
  tid = omp_get_thread_num()
  nt  = omp_get_num_threads()
  lo  = tid * n / nt
  hi  = (tid + 1) * n / nt
  s = 0.0d0
  do i = lo, hi - 1
     s = s + a(i)
  end do
  print "(a,i0,a,i0,a,i0,a,i0,a,f0.6)", &
       "thread ", tid, " of ", nt, ": range [", lo, ", ", hi, "), partial sum = ", s
  ! TODO: 上で始めた parallel 領域を閉じる (!$omp end parallel).
end program manual_partition
