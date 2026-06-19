program montecarlo
  use omp_lib
  implicit none
  integer(8) :: n, lo, hi, my_n, hits, i
  integer :: tid, nt
  integer(8) :: state
  real(8) :: x, y, pi
  character(len=32) :: arg

  ! 全体で投げる点の数 (コマンドライン引数, 既定 4,000,000)
  if (command_argument_count() >= 1) then
     call get_command_argument(1, arg)
     read(arg, *) n
  else
     n = 4000000_8
  end if

  ! BEGIN ANSWER: 下のブロックを !$omp parallel private(tid, nt, lo, hi, my_n, hits, i, state, x, y, pi) ... !$omp end parallel で囲み, 各スレッドが自分の担当分の点を投げて自分の π 推定値を表示するようにせよ.
  !$omp parallel private(tid, nt, lo, hi, my_n, hits, i, state, x, y, pi)
  ! END ANSWER
  tid = omp_get_thread_num()
  nt  = omp_get_num_threads()
  ! このスレッドの担当する点数 (n を T スレッドで分割)
  lo = tid * n / nt
  hi = (tid + 1) * n / nt
  my_n = hi - lo
  ! スレッドごとに異なる乱数種 (各スレッドが独立した LCG を持つ)
  state = int(tid + 1, 8)
  hits = 0
  do i = 1, my_n
     x = lcg(state)
     y = lcg(state)
     if (x * x + y * y < 1.0d0) then
        hits = hits + 1
     end if
  end do
  ! 単位正方形に対する 1/4 円の面積比 = π/4
  if (my_n > 0) then
     pi = 4.0d0 * real(hits, 8) / real(my_n, 8)
  else
     pi = 0.0d0
  end if
  print "(a,i0,a,i0,a,i0,a,f0.6)", &
       "thread ", tid, " of ", nt, ": ", my_n, " points, pi estimate = ", pi
  ! BEGIN ANSWER: 上で始めた parallel 領域を閉じる (!$omp end parallel).
  !$omp end parallel
  ! END ANSWER

contains

  ! 単純な線形合同法 (LCG). state を更新し [0,1) の乱数を返す.
  ! Fortran 組込みの乱数はスレッド安全とは限らないので自前で実装する.
  function lcg(s) result(r)
    integer(8), intent(inout) :: s
    real(8) :: r
    ! 2^31 を法とする LCG (Numerical Recipes 系の係数)
    s = mod(1103515245_8 * s + 12345_8, 2147483648_8)
    r = real(s, 8) / 2147483648.0d0
  end function lcg

end program montecarlo
