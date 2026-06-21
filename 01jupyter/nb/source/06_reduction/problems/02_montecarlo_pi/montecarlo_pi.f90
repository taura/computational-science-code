module rng_mod
contains
  ! 状態を持たない (カウンタベースの) 乱数: (seed,k) から [0,1) の値を決める純粋関数。
  ! 同じ (seed,k) なら必ず同じ値 → 並列化しても引かれる乱数列はスレッド数によらず同一。
  ! (教育用の簡単なハッシュ。M=2^31-1 未満で計算し, 途中の積も 64bit に収まる。)
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
end module rng_mod

program montecarlo_pi
  use rng_mod
  character(len=64) :: arg
  integer(8) :: n, i, count
  real(8) :: x, y, pi
  n = 100_8 * 1000_8 * 1000_8
  if (command_argument_count() >= 1) then
     call get_command_argument(1, arg); read (arg, *) n
  end if
  count = 0                      ! 単位円の 1/4 の内側に入った点数
  print "(a,i0)", "n = ", n
  ! 単位正方形 [0,1)x[0,1) に n 点を投げ, 半径 1 の円の内側に入った点を数える。
  ! 点 i は乱数列 i の 0,1 番目を x,y 座標に使う。
  ! TODO: 円内に入った点数を reduction(+:count) で集計して π を求めよ.
  ! BEGIN ANSWER
  !$omp parallel do private(x, y) reduction(+:count)
  ! END ANSWER
  do i = 0, n - 1
     x = draw_rand01(i, 0_8)
     y = draw_rand01(i, 1_8)
     if (x * x + y * y < 1.0d0) count = count + 1
  end do
  ! BEGIN ANSWER
  !$omp end parallel do
  ! END ANSWER
  pi = 4.0d0 * real(count, 8) / real(n, 8)
  print "(a,i0,a,i0)", "count = ", count, " / ", n
  print "(a,f0.6)", "pi ~= ", pi
end program montecarlo_pi
