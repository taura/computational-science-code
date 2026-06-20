module markov_mod
contains
  ! 状態を持たない乱数 (未使用だが慣例として置いておく): (seed,k) から [0,1)。
  function draw_rand01(seed, k) result(u)
    integer(8), intent(in) :: seed, k
    real(8) :: u
    integer(8), parameter :: M = 2147483647_8
    integer(8) :: x
    x = modulo(modulo(seed, M) * 2654435761_8 + modulo(k, M) + 1_8, M)
    x = modulo(ieor(x, ishft(x, -16)) * 1812433253_8, M)
    x = modulo(ieor(x, ishft(x, -13)) * 1664525_8,    M)
    x = modulo(ieor(x, ishft(x, -16)), M)
    u = real(x, 8) / real(M, 8)
  end function draw_rand01

  ! ワープ (はしご/すべり台) の行き先。from に該当しなければ d をそのまま返す。
  function warp(d, S) result(r)
    integer, intent(in) :: d, S
    integer :: r
    if (d == 3) then
       r = S / 2
    else if (d == S / 4) then
       r = S - 2
    else if (d == S/2 + 5) then
       r = 1
    else if (d == S - 7) then
       r = S / 3
    else
       r = d
    end if
  end function warp
end module markov_mod

program markov
  use markov_mod
  use omp_lib
  character(len=32) :: arg
  integer :: S, maxit, it, s_, t, roll, d, best, r, q, b
  logical :: used
  real(8) :: tol, total, diff, e, sm, t0, elapsed
  integer :: top(0:2)
  real(8), allocatable :: M(:,:), pi(:), pin(:)
  S = 1000; tol = 1d-10; maxit = 100000
  if (command_argument_count() >= 1) then
     call get_command_argument(1, arg); read (arg, *) S
  end if
  if (command_argument_count() >= 2) then
     call get_command_argument(2, arg); read (arg, *) tol
  end if
  if (command_argument_count() >= 3) then
     call get_command_argument(3, arg); read (arg, *) maxit
  end if

  ! 遷移行列 M (密) を構築。M(t,s) = マス s から t へ1ターンで移る確率。
  ! 各 s についてサイコロ 1..6 を振り d=mod(s+roll,S), ワープがあれば飛ばす。
  allocate(M(0:S-1, 0:S-1), pi(0:S-1), pin(0:S-1))
  M = 0.0d0
  do s_ = 0, S - 1
     do roll = 1, 6
        d = warp(modulo(s_ + roll, S), S)
        M(d, s_) = M(d, s_) + 1.0d0 / 6.0d0
     end do
  end do

  do s_ = 0, S - 1
     pi(s_) = 1.0d0 / S          ! 一様分布から開始
  end do

  ! べき乗法: 遷移行列を繰り返し掛けると定常分布に収束する (最大固有値 = 1)。
  t0 = omp_get_wtime()
  do it = 1, maxit
     ! TODO: 行 t ごとの行列ベクトル積を !$omp parallel do で並列化せよ (各 t は独立).
     do t = 0, S - 1
        sm = 0.0d0
        do s_ = 0, S - 1
           sm = sm + M(t, s_) * pi(s_)
        end do
        pin(t) = sm
     end do
     ! TODO: 上で始めた parallel do 領域を閉じる (!$omp end parallel do)。
     total = 0.0d0
     ! TODO: 総和を !$omp parallel do reduction(+:total) で並列化せよ.
     do t = 0, S - 1
        total = total + pin(t)
     end do
     ! TODO: 上で始めた parallel do 領域を閉じる (!$omp end parallel do)。
     diff = 0.0d0
     do t = 0, S - 1
        pin(t) = pin(t) / total              ! 正規化 (sum=1)
        e = abs(pin(t) - pi(t)); if (e > diff) diff = e
        pi(t) = pin(t)
     end do
     if (diff < tol) exit
  end do
  elapsed = omp_get_wtime() - t0

  ! 検算: sum(pi), 最も止まりやすいマスとその確率, 上位3マス
  sm = 0.0d0
  do s_ = 0, S - 1
     sm = sm + pi(s_)
  end do
  best = 0
  do s_ = 1, S - 1
     if (pi(s_) > pi(best)) best = s_
  end do

  top = -1
  do r = 0, 2
     b = -1
     do s_ = 0, S - 1
        used = .false.
        do q = 0, r - 1
           if (top(q) == s_) used = .true.
        end do
        if (used) cycle
        if (b < 0) then
           b = s_
        else if (pi(s_) > pi(b)) then
           b = s_
        end if
     end do
     top(r) = b
  end do

  print "(a,i0,a,i0,a,f0.10)", "S=", S, ", iters=", min(it, maxit), ", sum=", sm
  print "(a,i0,a,f0.6,a,f0.6)", &
       "最も止まりやすいマス=", best, " (確率 ", pi(best), "), 一様なら 1/S=", 1.0d0 / S
  print "(a,i0,a,f0.6,a,i0,a,f0.6,a,i0,a,f0.6,a)", &
       "上位3マス: ", top(0), "(", pi(top(0)), "), ", top(1), "(", pi(top(1)), &
       "), ", top(2), "(", pi(top(2)), ")"
  print "(a,f0.3,a)", "elapsed = ", elapsed, " sec"
end program markov
