module lcg_mod
contains
  ! a * b の下位 64bit を, 符号付き整数の overflow (Fortran では未定義動作) を
  ! 避けるため 32bit ずつに分割して計算する.
  function mulw(a, b) result(p)
    integer(8), intent(in) :: a, b
    integer(8) :: p, al, ah, bl, bh, mid
    integer(8), parameter :: mask = 4294967295_8     ! 2^32 - 1
    al = iand(a, mask); ah = ishft(a, -32)
    bl = iand(b, mask); bh = ishft(b, -32)
    mid = iand(al * bh + ah * bl, mask)   ! さらに上位は << 32 で消えるので捨てる
    p = al * bl + ishft(mid, 32)
  end function mulw

  ! 反復番号 idx から決まる擬似乱数 (splitmix64) を [0,1) の double で返す.
  ! 毎回 idx から計算するので, スレッドごとの状態を持たず並列化しやすい.
  function lcg01(idx) result(r)
    integer(8), intent(in) :: idx
    integer(8) :: x
    real(8) :: r
    x = idx + 1
    x = mulw(x, 6364136223846793005_8) + 1442695040888963407_8
    x = ieor(x, ishft(x, -30)); x = mulw(x, -4658895280553007687_8)
    x = ieor(x, ishft(x, -27)); x = mulw(x, -7723592293110705685_8)
    x = ieor(x, ishft(x, -31))
    ! 上位 53 bit を [0,1) の double に
    r = real(ishft(x, -11), 8) * (1.0d0 / 9007199254740992.0d0)
  end function lcg01
end module lcg_mod

program montecarlo_pi
  use lcg_mod
  character(len=64) :: arg
  integer(8) :: n, i, count
  real(8) :: x, y, pi
  n = 100_8 * 1000_8 * 1000_8
  if (command_argument_count() >= 1) then
     call get_command_argument(1, arg); read (arg, *) n
  end if
  count = 0                      ! 単位円の 1/4 の内側に入った点数
  print "(a,i0)", "n = ", n
  ! 単位正方形 [0,1)x[0,1) に n 点を投げ, 半径 1 の円の内側に入った点を数える.
  ! TODO: 円内に入った点数を reduction(+:count) で集計して π を求めよ.
  do i = 0, n - 1
     x = lcg01(2 * i)
     y = lcg01(2 * i + 1)
     if (x * x + y * y < 1.0d0) count = count + 1
  end do
  ! TODO: 上で始めた parallel do 領域を閉じる (!$omp end parallel do).
  pi = 4.0d0 * real(count, 8) / real(n, 8)
  print "(a,i0,a,i0)", "count = ", count, " / ", n
  print "(a,f0.6)", "pi ~= ", pi
end program montecarlo_pi
