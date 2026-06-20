program montecarlo
  use omp_lib
  implicit none
  integer(8) :: n, lo, hi, my_n, hits, i
  integer :: tid, nt
  real(8) :: x, y, pi
  character(len=32) :: arg

  ! 全体で投げる点の数 (コマンドライン引数, 既定 4,000,000)
  if (command_argument_count() >= 1) then
     call get_command_argument(1, arg)
     read(arg, *) n
  else
     n = 4000000_8
  end if

  ! BEGIN ANSWER: 下のブロックを !$omp parallel private(tid, nt, lo, hi, my_n, hits, i, x, y, pi) ... !$omp end parallel で囲み, 各スレッドが自分の担当分の点を投げて自分の π 推定値を表示するようにせよ.
  !$omp parallel private(tid, nt, lo, hi, my_n, hits, i, x, y, pi)
  ! END ANSWER
  tid = omp_get_thread_num()
  nt  = omp_get_num_threads()
  ! このスレッドの担当する点の範囲 (全体 n 点を T スレッドで分割)
  lo = tid * n / nt
  hi = (tid + 1) * n / nt
  my_n = hi - lo
  hits = 0
  do i = lo, hi - 1
     x = draw_rand01(i, 0_8)         ! 点 i の x 座標
     y = draw_rand01(i, 1_8)         ! 点 i の y 座標
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

  ! 状態を持たない (カウンタベースの) 乱数: (seed,k) から [0,1) の値を決める純粋関数。
  ! 点 i の座標を draw_rand01(i,0), draw_rand01(i,1) で決めるので, どのスレッドが
  ! 担当しても点 i の位置は同じ (共有状態が無いので競合しない)。
  function draw_rand01(seed, k) result(u)
    integer(8), intent(in) :: seed, k
    real(8) :: u
    integer(8), parameter :: M = 2147483647_8   ! 2^31 - 1
    integer(8) :: x
    x = modulo(modulo(seed, M) * 2654435761_8 + modulo(k, M) + 1_8, M)
    x = modulo(ieor(x, ishft(x, -16)) * 1812433253_8, M)
    x = modulo(ieor(x, ishft(x, -13)) * 1664525_8,    M)
    x = modulo(ieor(x, ishft(x, -16)), M)
    u = real(x, 8) / real(M, 8)        ! [0,1)
  end function draw_rand01

end program montecarlo
