module nbody_mod
contains
  ! 状態を持たない乱数 (初期配置の再現性のため): (seed,k) から [0,1)。
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

  ! 各粒子 i に働く加速度 = 他の全粒子 j からの重力の和 (直接法 O(N^2))。
  ! ソフトニング eps で近距離の発散を防ぐ。G=1。(j=i の項は dx=0 なので寄与 0。)
  subroutine compute_acc(N, pos, mass, acc, eps)
    integer, intent(in) :: N
    real(8), intent(in) :: pos(3,N), mass(N), eps
    real(8), intent(out) :: acc(3,N)
    real(8) :: eps2, xi, yi, zi, ax, ay, az, dx, dy, dz, r2, inv, f
    integer :: i, j
    eps2 = eps * eps
    ! TODO: 各粒子 i のループを並列化する (i ごとに独立)。
    ! BEGIN ANSWER
    !$omp parallel do private(xi,yi,zi,ax,ay,az,dx,dy,dz,r2,inv,f,j)
    ! END ANSWER
    do i = 1, N
       xi = pos(1,i); yi = pos(2,i); zi = pos(3,i)
       ax = 0.0d0; ay = 0.0d0; az = 0.0d0
       do j = 1, N
          dx = pos(1,j) - xi; dy = pos(2,j) - yi; dz = pos(3,j) - zi
          r2 = dx*dx + dy*dy + dz*dz + eps2
          inv = 1.0d0 / (r2 * sqrt(r2))      ! 1/r^3 (ソフトニング込み)
          f = mass(j) * inv
          ax = ax + f*dx; ay = ay + f*dy; az = az + f*dz
       end do
       acc(1,i) = ax; acc(2,i) = ay; acc(3,i) = az
    end do
    ! BEGIN ANSWER
    !$omp end parallel do
    ! END ANSWER
  end subroutine compute_acc

  ! 全エネルギー = 運動エネルギー + 位置エネルギー (検算用)
  function energy(N, pos, vel, mass, eps) result(E)
    integer, intent(in) :: N
    real(8), intent(in) :: pos(3,N), vel(3,N), mass(N), eps
    real(8) :: E, eps2, KE, PE, dx, dy, dz
    integer :: i, j
    eps2 = eps * eps; KE = 0.0d0; PE = 0.0d0
    do i = 1, N
       KE = KE + 0.5d0 * mass(i) * (vel(1,i)**2 + vel(2,i)**2 + vel(3,i)**2)
       do j = i + 1, N
          dx = pos(1,j)-pos(1,i); dy = pos(2,j)-pos(2,i); dz = pos(3,j)-pos(3,i)
          PE = PE - mass(i) * mass(j) / sqrt(dx*dx + dy*dy + dz*dz + eps2)
       end do
    end do
    E = KE + PE
  end function energy
end module nbody_mod

program nbody
  use nbody_mod
  use omp_lib
  character(len=32) :: arg
  integer :: N, steps, t, i
  real(8) :: dt, eps, E0, E1, t0, elapsed
  real(8), allocatable :: pos(:,:), vel(:,:), acc(:,:), mass(:)
  N = 2000; steps = 100; dt = 0.001d0; eps = 0.05d0
  if (command_argument_count() >= 1) then
     call get_command_argument(1, arg); read (arg, *) N
  end if
  if (command_argument_count() >= 2) then
     call get_command_argument(2, arg); read (arg, *) steps
  end if

  allocate(pos(3,N), vel(3,N), acc(3,N), mass(N))
  vel = 0.0d0
  ! 初期条件: [-1,1]^3 にランダムに配置, 速度 0, 質量は等しく合計 1。
  do i = 1, N
     mass(i) = 1.0d0 / N
     pos(1,i) = 2.0d0 * draw_rand01(int(i-1,8), 0_8) - 1.0d0
     pos(2,i) = 2.0d0 * draw_rand01(int(i-1,8), 1_8) - 1.0d0
     pos(3,i) = 2.0d0 * draw_rand01(int(i-1,8), 2_8) - 1.0d0
  end do

  E0 = energy(N, pos, vel, mass, eps)
  t0 = omp_get_wtime()
  do t = 1, steps
     call compute_acc(N, pos, mass, acc, eps)
     ! シンプレクティック・オイラー法 (v を更新してから x を更新)
     vel = vel + acc * dt
     pos = pos + vel * dt
  end do
  elapsed = omp_get_wtime() - t0
  E1 = energy(N, pos, vel, mass, eps)

  print "(a,i0,a,i0,a,es13.6,a,es13.6,a,es9.2,a)", &
       "N=", N, ", steps=", steps, ": エネルギー ", E0, " -> ", E1, &
       " (相対変化 ", abs((E1 - E0) / E0), ")"
  print "(a,f0.3,a)", "elapsed = ", elapsed, " sec"
end program nbody
