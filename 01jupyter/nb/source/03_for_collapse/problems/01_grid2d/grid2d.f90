program grid2d
  use omp_lib
  integer, parameter :: N = 4
  real(8) :: a(0:N-1, 0:N-1)
  integer :: i, j
  ! BEGIN ANSWER: 下の二重ループを !$omp parallel do collapse(2) で始め, 二重ループ全体を複数スレッドに分担させよ.
  !$omp parallel do collapse(2)
  ! END ANSWER
  do i = 0, N - 1
     do j = 0, N - 1
        a(i, j) = i * 10 + j
        print "(a,i0,a,i0,a,f0.6,a,i0,a)", &
             "a(", i, ",", j, ") = ", a(i, j), "  (thread ", omp_get_thread_num(), ")"
     end do
  end do
  ! BEGIN ANSWER: 上で始めた parallel do 領域を閉じる (!$omp end parallel do).
  !$omp end parallel do
  ! END ANSWER
end program grid2d
