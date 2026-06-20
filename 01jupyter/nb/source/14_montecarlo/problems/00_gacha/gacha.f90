module gacha_mod
contains
  ! 試行ごとに独立な乱数生成器 (MINSTD 法). 16807*s は 64bit に収まる.
  ! s を更新し, [0,N) の整数を返す.
  function draw(s, N) result(k)
    integer(8), intent(inout) :: s
    integer, intent(in) :: N
    integer :: k
    s = modulo(16807_8 * s, 2147483647_8)
    k = int(modulo(s, int(N, 8)))
  end function draw

  ! 1試行: N 種類が等確率で出るとき, 全種類そろうまでに引いた回数.
  ! collected を 64bit 整数のビットマスクで管理 (N <= 62 を想定).
  function one_trial(N, seed) result(draws)
    integer, intent(in) :: N
    integer(8), intent(in) :: seed
    integer(8) :: draws, s, got, full
    integer :: k
    s = modulo(seed, 2147483647_8)
    if (s <= 0) s = s + 2147483646_8
    got = 0_8
    full = ishft(1_8, N) - 1_8
    draws = 0_8
    do while (got /= full)
       k = draw(s, N)
       got = ior(got, ishft(1_8, k))
       draws = draws + 1_8
    end do
  end function one_trial
end module gacha_mod

program gacha
  use gacha_mod
  character(len=32) :: arg
  integer :: N, k
  integer(8) :: T, t_
  real(8) :: sum, sumsq, mean, var, H
  N = 10
  T = 1000000_8
  if (command_argument_count() >= 1) then
     call get_command_argument(1, arg); read (arg, *) N
  end if
  if (command_argument_count() >= 2) then
     call get_command_argument(2, arg); read (arg, *) T
  end if
  sum = 0.0d0; sumsq = 0.0d0

  ! T 回の試行は互いに独立. 各試行の引き回数を集計する.
  ! BEGIN ANSWER: 各試行は独立なので !$omp parallel do reduction(+:sum,sumsq) で並列化・集計せよ.
  !$omp parallel do reduction(+:sum,sumsq)
  ! END ANSWER
  do t_ = 1, T
     block
       integer(8) :: d
       d = one_trial(N, t_ * 2654435761_8 + 12345_8)
       sum = sum + real(d, 8)
       sumsq = sumsq + real(d, 8) * real(d, 8)
     end block
  end do
  ! BEGIN ANSWER: 上の parallel do を閉じる !$omp end parallel do を書け.
  !$omp end parallel do
  ! END ANSWER

  mean = sum / T
  var  = sumsq / T - mean * mean
  ! 理論期待値 = N * H_N
  H = 0.0d0
  do k = 1, N
     H = H + 1.0d0 / k
  end do
  print "(a,i0,a,i0,a,f0.3,a,f0.3,a,f0.3)", &
       "N=", N, ", trials=", T, ": 平均 ", mean, " 回 (理論 ", N * H, "), 標準偏差 ", sqrt(var)
end program gacha
