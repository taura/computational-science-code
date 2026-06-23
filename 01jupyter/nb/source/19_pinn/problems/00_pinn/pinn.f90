module pinn_mod
  real(8), parameter :: PI = 3.14159265358979323846d0
contains
  ! 状態を持たない乱数 (パラメータ初期化用): (seed,k) から [0,1)。
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

  ! 損失をパラメータ配列 p (長さ 3H: a=p(1..H), w=p(H+1..2H), b=p(2H+1..3H)) の
  ! 純関数として計算する (スレッドセーフ)。xs は格子点 (長さ M)。
  function loss_fn(H, p, M, xs, lambda_bc) result(L)
    integer, intent(in) :: H, M
    real(8), intent(in) :: p(3*H), xs(M), lambda_bc
    real(8) :: L, res, upp, x, z, t, s2, f, r, u0, u1
    integer :: i, k
    res = 0.0d0
    do i = 1, M
       x = xs(i)
       upp = 0.0d0
       do k = 1, H
          z = p(H+k)*x + p(2*H+k)
          t = tanh(z)
          s2 = 1.0d0 - t*t
          upp = upp + p(k) * p(H+k)*p(H+k) * (-2.0d0*t*s2)
       end do
       f = PI*PI*sin(PI*x)
       r = -upp - f
       res = res + r*r
    end do
    res = res / real(M, 8)
    u0 = 0.0d0; u1 = 0.0d0
    do k = 1, H
       u0 = u0 + p(k) * tanh(p(2*H+k))
       u1 = u1 + p(k) * tanh(p(H+k) + p(2*H+k))
    end do
    L = res + lambda_bc * (u0*u0 + u1*u1)
  end function loss_fn
end module pinn_mod

! 物理情報ニューラルネット (PINN) で -u''(x)=f(x), u(0)=u(1)=0 を「NNで」解く。
! 1隠れ層・tanh なので u'' は解析的 (autodiff 不要)。厳密解 u*=sin(pi x), f=pi^2 sin(pi x)。
! パラメータについての差分勾配の 3H 個の評価を parallel do で分担する。
! 差分法(B系)・CG(G1)と同じ問題を全く別の道具(機械学習)で解く, 最上級の発展課題。
program pinn
  use pinn_mod
  use omp_lib
  character(len=32) :: arg
  integer :: H, M, steps, it, j, q, k, NP, NC, i
  real(8) :: lr, lambda_bc, eps, lp, lm, L, x, u, e, maxerr, t0, elapsed
  real(8), allocatable :: xs(:), p(:), grad(:), pp(:)

  H = 16; M = 60; steps = 4000; lr = 0.01d0
  if (command_argument_count() >= 1) then
     call get_command_argument(1, arg); read (arg, *) H
  end if
  if (command_argument_count() >= 2) then
     call get_command_argument(2, arg); read (arg, *) M
  end if
  if (command_argument_count() >= 3) then
     call get_command_argument(3, arg); read (arg, *) steps
  end if
  if (command_argument_count() >= 4) then
     call get_command_argument(4, arg); read (arg, *) lr
  end if
  lambda_bc = 10.0d0; eps = 1.0d-5; NP = 3*H

  ! コロケーション点 x_i = (i-0.5)/M を (0,1) に等間隔配置
  allocate(xs(M))
  do i = 1, M
     xs(i) = (real(i,8) - 0.5d0) / real(M, 8)
  end do

  ! パラメータ p(3H) = {a, w, b} を小さな乱数で初期化
  allocate(p(NP), grad(NP))
  do k = 1, H
     p(k)     = (draw_rand01(int(k-1,8), 1_8) - 0.5d0) * 0.2d0   ! a
     p(H+k)   = (draw_rand01(int(k-1,8), 2_8) - 0.5d0) * 4.0d0   ! w
     p(2*H+k) = (draw_rand01(int(k-1,8), 3_8) - 0.5d0) * 2.0d0   ! b
  end do

  t0 = omp_get_wtime()
  do it = 0, steps - 1
     ! パラメータについての差分勾配。各 j は独立 (loss は純関数なので各スレッドが
     ! 自分のコピー pp を摂動して評価する)。
     ! TODO: 3H 個のパラメータの差分勾配ループを並列化する (各 j は独立, loss_fn を2回評価)。
     ! BEGIN ANSWER
     !$omp parallel do private(j,q,pp,lp,lm)
     ! END ANSWER
     do j = 1, NP
        allocate(pp(NP))
        do q = 1, NP
           pp(q) = p(q)
        end do
        pp(j) = p(j) + eps; lp = loss_fn(H, pp, M, xs, lambda_bc)
        pp(j) = p(j) - eps; lm = loss_fn(H, pp, M, xs, lambda_bc)
        grad(j) = (lp - lm) / (2.0d0 * eps)
        deallocate(pp)
     end do
     ! BEGIN ANSWER
     !$omp end parallel do
     ! END ANSWER
     do j = 1, NP
        p(j) = p(j) - lr * grad(j)
     end do

     if (mod(it, 1000) == 0 .or. it == steps - 1) then
        L = loss_fn(H, p, M, xs, lambda_bc)
        print "(a,i4,a,es13.6)", "step ", it, ": loss=", L
     end if
  end do
  elapsed = omp_get_wtime() - t0

  ! 検算: 学習した u(x) を厳密解 sin(pi x) と比べる
  maxerr = 0.0d0; NC = 101
  do i = 0, NC - 1
     x = real(i, 8) / real(NC - 1, 8)
     u = 0.0d0
     do k = 1, H
        u = u + p(k) * tanh(p(H+k)*x + p(2*H+k))
     end do
     e = abs(u - sin(PI*x))
     if (e > maxerr) maxerr = e
  end do
  print "(a,i0,a,i0,a,i0,a,f0.4)", "H=", H, ", M=", M, ", steps=", steps, ", lr=", lr
  print "(a,es12.4)", "final max|u - sin(pi x)| = ", maxerr
  print "(a,f0.3,a)", "elapsed = ", elapsed, " sec"
end program pinn
