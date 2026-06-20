module boids_mod
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

  ! 群れの整列度 (polarization): 全個体の進行方向の平均の大きさ (0=バラバラ, 1=揃った)。
  function polarization(N, vx, vy) result(P)
    integer, intent(in) :: N
    real(8), intent(in) :: vx(N), vy(N)
    real(8) :: P, sx, sy, s
    integer :: i
    sx = 0.0d0; sy = 0.0d0
    do i = 1, N
       s = sqrt(vx(i)**2 + vy(i)**2)
       sx = sx + vx(i)/s; sy = sy + vy(i)/s
    end do
    P = sqrt(sx*sx + sy*sy) / N
  end function polarization
end module boids_mod

program boids
  use boids_mod
  use omp_lib
  character(len=32) :: arg
  integer :: N, steps, t, i, j, cnt
  real(8) :: box, R, Rs, wc, wa, ws, speed, dt, P0
  real(8) :: cx, cy, avx, avy, sx, sy, ax, ay, dx, dy, d2, nvx, nvy, s, ang, t0, elapsed
  real(8), allocatable :: px(:), py(:), vx(:), vy(:), qx(:), qy(:), ux(:), uy(:), tmp(:)
  N = 1000; steps = 300
  box = 30.0d0; R = 15.0d0; Rs = 2.0d0
  wc = 0.01d0; wa = 0.2d0; ws = 0.05d0; speed = 1.0d0; dt = 1.0d0
  if (command_argument_count() >= 1) then
     call get_command_argument(1, arg); read (arg, *) N
  end if
  if (command_argument_count() >= 2) then
     call get_command_argument(2, arg); read (arg, *) steps
  end if

  allocate(px(N), py(N), vx(N), vy(N), qx(N), qy(N), ux(N), uy(N))
  do i = 1, N
     px(i) = box * draw_rand01(int(i-1,8), 0_8)
     py(i) = box * draw_rand01(int(i-1,8), 1_8)
     ang = 2.0d0 * 3.14159265358979d0 * draw_rand01(int(i-1,8), 2_8)
     vx(i) = cos(ang); vy(i) = sin(ang)
  end do
  P0 = polarization(N, vx, vy)

  t0 = omp_get_wtime()
  do t = 1, steps
     ! 各個体 i を, 近傍 j を見て更新する (全体で O(N^2))。
     ! TODO: 各個体 i のループを !$omp parallel do (i ごとに独立) で並列化せよ。
     do i = 1, N
        cx = 0; cy = 0; avx = 0; avy = 0; sx = 0; sy = 0; cnt = 0
        do j = 1, N
           if (j == i) cycle
           dx = px(j) - px(i); dy = py(j) - py(i); d2 = dx*dx + dy*dy
           if (d2 < R*R) then
              cx = cx + px(j); cy = cy + py(j); avx = avx + vx(j); avy = avy + vy(j); cnt = cnt + 1
              if (d2 < Rs*Rs) then     ! 分離: 近すぎる相手から離れる
                 sx = sx + (px(i) - px(j)); sy = sy + (py(i) - py(j))
              end if
           end if
        end do
        ax = 0; ay = 0
        if (cnt > 0) then
           cx = cx/cnt; cy = cy/cnt; avx = avx/cnt; avy = avy/cnt
           ax = ax + wc*(cx - px(i)) + wa*(avx - vx(i))   ! 結合 + 整列
           ay = ay + wc*(cy - py(i)) + wa*(avy - vy(i))
        end if
        ax = ax + ws*sx; ay = ay + ws*sy                  ! 分離
        nvx = vx(i) + ax; nvy = vy(i) + ay
        s = sqrt(nvx*nvx + nvy*nvy); if (s < 1d-9) s = 1.0d0
        nvx = nvx/s*speed; nvy = nvy/s*speed               ! 速さは一定に
        ux(i) = nvx; uy(i) = nvy
        qx(i) = modulo(px(i) + nvx*dt, box)                ! 周期境界
        qy(i) = modulo(py(i) + nvy*dt, box)
     end do
     ! TODO: 上で始めた parallel do 領域を閉じる (!$omp end parallel do)。
     ! 現在 <-> 次 を入れ替える
     call move_alloc(px, tmp); call move_alloc(qx, px); call move_alloc(tmp, qx)
     call move_alloc(py, tmp); call move_alloc(qy, py); call move_alloc(tmp, qy)
     call move_alloc(vx, tmp); call move_alloc(ux, vx); call move_alloc(tmp, ux)
     call move_alloc(vy, tmp); call move_alloc(uy, vy); call move_alloc(tmp, uy)
  end do
  elapsed = omp_get_wtime() - t0

  print "(a,i0,a,i0,a,f0.4,a,f0.4,a)", &
       "N=", N, ", steps=", steps, ": 整列度 ", P0, " -> ", polarization(N, vx, vy), &
       " (1 に近いほど群れが揃った)"
  print "(a,f0.3,a)", "elapsed = ", elapsed, " sec"
end program boids
