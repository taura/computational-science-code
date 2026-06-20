module gacha_mod
  integer(8), parameter :: M = 2147483647_8   ! 2^31 - 1
contains
  ! 状態を持たない (カウンタベースの) 乱数。
  ! mix(x) は「整数 x → よく混ざった整数」を返す純粋関数。
  ! 同じ入力なら必ず同じ値 → スレッドで分担しても引かれる乱数列は変わらない。
  ! (教育用の簡単なハッシュ。途中の積も 64bit に収まり桁あふれしない。)
  function mix(x0) result(x)
    integer(8), intent(in) :: x0
    integer(8) :: x
    x = modulo(ieor(x0, ishft(x0, -16)) * 1812433253_8, M)
    x = modulo(ieor(x,  ishft(x,  -13)) * 1664525_8,    M)
    x = modulo(ieor(x,  ishft(x,  -16)), M)
  end function mix

  ! t 回目の試行の k 回目の引きで出る景品番号 (0..N-1)
  function draw(t, k, N) result(idx)
    integer(8), intent(in) :: t, k
    integer, intent(in) :: N
    integer :: idx
    integer(8) :: key
    key = modulo((t + 1) * 2654435761_8 + (k + 1), M)
    idx = int(modulo(mix(key), int(N, 8)))
  end function draw

  ! 1試行: 全種類そろうまでに引いた回数 (そろった種類を 64bit マスクで管理, N <= 62)
  function one_trial(N, t) result(draws)
    integer, intent(in) :: N
    integer(8), intent(in) :: t
    integer(8) :: draws, got, full, k
    integer :: idx
    got = 0_8
    full = ishft(1_8, N) - 1_8
    k = 0_8
    do while (got /= full)
       idx = draw(t, k, N)
       got = ior(got, ishft(1_8, idx))
       k = k + 1_8
    end do
    draws = k
  end function one_trial
end module gacha_mod

program gacha
  use gacha_mod
  character(len=32) :: arg
  integer :: N, k
  integer(8) :: T, t_, total, totalsq, d
  real(8) :: mean, var, H
  N = 10
  T = 1000000_8
  if (command_argument_count() >= 1) then
     call get_command_argument(1, arg); read (arg, *) N
  end if
  if (command_argument_count() >= 2) then
     call get_command_argument(2, arg); read (arg, *) T
  end if
  ! 引き回数は整数なので整数で集計する → 足す順番によらず答えが完全に一致する
  total = 0_8; totalsq = 0_8

  ! T 回の試行は互いに独立。各試行の引き回数を集計する。
  ! TODO: 各試行は独立なので !$omp parallel do reduction(+:total,totalsq) で並列化・集計せよ.
  do t_ = 0, T - 1
     d = one_trial(N, t_)
     total = total + d
     totalsq = totalsq + d * d
  end do
  ! TODO: 上の parallel do を閉じる !$omp end parallel do を書け.

  mean = real(total, 8) / T
  var  = real(totalsq, 8) / T - mean * mean
  ! 理論期待値 = N * H_N
  H = 0.0d0
  do k = 1, N
     H = H + 1.0d0 / k
  end do
  print "(a,i0,a,i0,a,f0.3,a,f0.3,a,f0.3)", &
       "N=", N, ", trials=", T, ": 平均 ", mean, " 回 (理論 ", N * H, "), 標準偏差 ", sqrt(var)
end program gacha
