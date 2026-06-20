module gacha_mod
contains
  ! 状態を持たない (カウンタベースの) 乱数: たった1つの純粋関数。
  ! draw_rand(seed, k, N) は 0..N-1 の整数を返す。
  ! - seed は「どの乱数列(ストリーム)を使うか」を選ぶ番号 (この問題では試行ごとに変える)。
  ! - k は「その列の何番目を取り出すか」。同じ seed でも k が違えば別の値。
  ! - 同じ (seed,k) なら必ず同じ値 → スレッドで分担しても乱数列はスレッド数によらず同一。
  ! (教育用の簡単なハッシュ。M=2^31-1 未満で計算し, 途中の積も 64bit に収まる。)
  function draw_rand(seed, k, N) result(idx)
    integer(8), intent(in) :: seed, k
    integer, intent(in) :: N
    integer :: idx
    integer(8), parameter :: M = 2147483647_8   ! 2^31 - 1
    integer(8) :: x
    x = modulo(modulo(seed, M) * 2654435761_8 + modulo(k, M) + 1_8, M)   ! seed と k を1つにまとめる
    x = modulo(ieor(x, ishft(x, -16)) * 1812433253_8, M)
    x = modulo(ieor(x, ishft(x, -13)) * 1664525_8,    M)
    x = modulo(ieor(x, ishft(x, -16)), M)
    idx = int(modulo(x, int(N, 8)))
  end function draw_rand

  ! 1試行: 全種類そろうまでに引いた回数 (そろった種類を 64bit マスクで管理, N <= 62)
  ! seed に試行番号を渡し, k=0,1,2,... と引いていく。
  function one_trial(N, seed) result(draws)
    integer, intent(in) :: N
    integer(8), intent(in) :: seed
    integer(8) :: draws, got, full, k
    integer :: idx
    got = 0_8
    full = ishft(1_8, N) - 1_8
    k = 0_8
    do while (got /= full)
       idx = draw_rand(seed, k, N)
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
