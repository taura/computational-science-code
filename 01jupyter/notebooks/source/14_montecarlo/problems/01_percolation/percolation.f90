module percolation_mod
contains
  ! 状態を持たない (カウンタベースの) 乱数: (seed,k) から [0,1) の値を決める純粋関数。
  ! セル k の開閉を draw_rand01(seed=試行番号, k=セル番号) で決めるので, どのスレッドが
  ! 担当しても同じ格子になり, スレッド数によらず結果が一致する (共有状態なし=競合なし)。
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

  ! 1試行: L×L の各セルを確率 p で「開」にした格子で, 上端の開セルから
  ! 下端の開セルへたどり着けるか。たどり着ければ 1, さもなくば 0。深さ優先探索で判定。
  ! セル番号は 0..L*L-1 (idx = r*L + c, r,c は 0 始まり)。
  function one_trial(L, p, seed) result(perc)
    integer, intent(in) :: L
    real(8), intent(in) :: p
    integer(8), intent(in) :: seed
    integer :: perc
    integer :: n, i, c, r, sp, idx, d, nr, nc, nidx
    logical, allocatable :: open(:), vis(:)
    integer, allocatable :: stk(:)
    integer :: dr(4), dc(4)
    dr = (/ -1, 1, 0, 0 /)
    dc = (/ 0, 0, -1, 1 /)
    n = L * L
    allocate(open(0:n-1), vis(0:n-1), stk(0:n-1))
    do i = 0, n - 1
       open(i) = (draw_rand01(seed, int(i, 8)) < p)
    end do
    vis = .false.
    sp = 0
    do c = 0, L - 1                 ! 上端 (行0) の開セルを出発点に
       if (open(c)) then
          vis(c) = .true.; stk(sp) = c; sp = sp + 1
       end if
    end do
    perc = 0
    do while (sp > 0)
       sp = sp - 1; idx = stk(sp)
       r = idx / L; c = mod(idx, L)
       if (r == L - 1) then
          perc = 1; exit           ! 下端に到達 = 浸透
       end if
       do d = 1, 4
          nr = r + dr(d); nc = c + dc(d)
          if (nr < 0 .or. nr >= L .or. nc < 0 .or. nc >= L) cycle
          nidx = nr * L + nc
          if (open(nidx) .and. .not. vis(nidx)) then
             vis(nidx) = .true.; stk(sp) = nidx; sp = sp + 1
          end if
       end do
    end do
    deallocate(open, vis, stk)
  end function one_trial
end module percolation_mod

program percolation
  use percolation_mod
  character(len=32) :: arg
  integer :: L
  real(8) :: p
  integer(8) :: T, t_, perc
  L = 128
  p = 0.6d0
  T = 2000_8
  if (command_argument_count() >= 1) then
     call get_command_argument(1, arg); read (arg, *) L
  end if
  if (command_argument_count() >= 2) then
     call get_command_argument(2, arg); read (arg, *) p
  end if
  if (command_argument_count() >= 3) then
     call get_command_argument(3, arg); read (arg, *) T
  end if
  perc = 0_8

  ! T 回の試行は互いに独立。浸透した回数を数える。
  ! 試行ごとに探索量が違うので schedule(dynamic) が有効。
  ! TODO: 各試行は独立。!$omp parallel do reduction(+:perc) schedule(dynamic) で並列化・集計せよ.
  do t_ = 0, T - 1
     perc = perc + one_trial(L, p, t_)
  end do
  ! TODO: 上で始めた parallel do 領域を閉じる (!$omp end parallel do).

  print "(a,i0,a,f0.3,a,i0,a,f0.4)", &
       "L=", L, ", p=", p, ", trials=", T, ": 浸透確率 = ", real(perc, 8) / T
end program percolation
