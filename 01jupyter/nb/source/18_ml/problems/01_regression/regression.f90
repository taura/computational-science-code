module logreg_mod
contains
  ! 状態を持たない乱数 (合成データ生成用): (seed,k) から [0,1)。
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

  function sigmoid(z) result(s)
    real(8), intent(in) :: z
    real(8) :: s
    s = 1.0d0 / (1.0d0 + exp(-z))
  end function sigmoid
end module logreg_mod

! 勾配降下法でロジスティック回帰を学習する。
! 予測 p = sigmoid(w・x)。損失は二値クロスエントロピー。
! 勾配 grad(jd) = (1/N) Σ_i (sigmoid(w・x_i) - y_i) * x_i(jd)。
! w を 0 から始め, 各エポックで w(jd) -= lr * grad(jd) と更新する。
! 合成データは線形分離可能なので, 学習が進むと正解率が上がっていくのを観察できる。
! 並列化対象は「全サンプルにわたる予測・誤差の和」(行列積 + reduction)。
program regression
  use logreg_mod
  use omp_lib
  character(len=32) :: arg
  integer :: D, E, ep, jd, predc
  integer(8) :: N, i
  real(8) :: lr, loss, z, p, eps, score, xv, g, t0, elapsed
  integer(8) :: correct
  real(8), allocatable :: w_true(:), X(:), w(:), grad(:), err(:)
  integer, allocatable :: y(:)

  N = 200000_8; D = 20; E = 200; lr = 1.0d0
  if (command_argument_count() >= 1) then
     call get_command_argument(1, arg); read (arg, *) N
  end if
  if (command_argument_count() >= 2) then
     call get_command_argument(2, arg); read (arg, *) D
  end if
  if (command_argument_count() >= 3) then
     call get_command_argument(3, arg); read (arg, *) E
  end if
  if (command_argument_count() >= 4) then
     call get_command_argument(4, arg); read (arg, *) lr
  end if

  ! 真の重み w_true (= 学習で復元したい正解), 範囲 [-1,1)。添字 0 始まり。
  allocate(w_true(0:D-1), w(0:D-1), grad(0:D-1))
  allocate(X(0:N*D-1), y(0:N-1), err(0:N-1))
  do jd = 0, D - 1
     w_true(jd) = draw_rand01(int(jd,8), 7_8) * 2.0d0 - 1.0d0
  end do

  ! 特徴 x[i][jd] (中心化), ラベル y(i) = (w_true・x_i > 0)。線形分離可能。
  do i = 0, N - 1
     score = 0.0d0
     do jd = 0, D - 1
        xv = draw_rand01(i, int(jd,8)) - 0.5d0
        X(i*D + jd) = xv
        score = score + w_true(jd) * xv
     end do
     if (score > 0.0d0) then
        y(i) = 1
     else
        y(i) = 0
     end if
  end do

  do jd = 0, D - 1
     w(jd) = 0.0d0
  end do
  eps = 1.0d-12

  t0 = omp_get_wtime()
  do ep = 0, E - 1
     loss = 0.0d0
     correct = 0_8
     ! 各サンプルの予測 p = sigmoid(w・x_i), 誤差 err(i) = p - y_i,
     ! 損失・正解数を集計する。各サンプルは独立なので並列化できる。
     ! BEGIN ANSWER: サンプルのループを !$omp parallel do reduction(+:loss,correct) で並列化せよ.
     !$omp parallel do private(i, jd, z, p, predc) reduction(+:loss,correct)
     ! END ANSWER
     do i = 0, N - 1
        z = 0.0d0
        do jd = 0, D - 1
           z = z + w(jd) * X(i*D + jd)
        end do
        p = sigmoid(z)
        err(i) = p - real(y(i), 8)
        if (y(i) == 1) then
           loss = loss - log(p + eps)
        else
           loss = loss - log(1.0d0 - p + eps)
        end if
        if (p > 0.5d0) then
           predc = 1
        else
           predc = 0
        end if
        if (predc == y(i)) correct = correct + 1
     end do
     ! BEGIN ANSWER: 上で始めた parallel do 領域を閉じる (!$omp end parallel do)。
     !$omp end parallel do
     ! END ANSWER
     loss = loss / real(N, 8)

     ! 勾配 grad(jd) = (1/N) Σ_i err(i) * x_i(jd)。特徴ごとに独立なので jd で並列化 (競合なし)。
     !$omp parallel do private(jd, i, g)
     do jd = 0, D - 1
        g = 0.0d0
        do i = 0, N - 1
           g = g + err(i) * X(i*D + jd)
        end do
        grad(jd) = g / real(N, 8)
     end do
     !$omp end parallel do
     ! 重みの更新
     do jd = 0, D - 1
        w(jd) = w(jd) - lr * grad(jd)
     end do

     if (mod(ep, 50) == 0 .or. ep == E - 1) then
        print "(a,i3,a,f0.4,a,f0.2,a)", &
             "epoch ", ep, ": loss=", loss, ", acc=", 100.0d0 * correct / N, "%"
     end if
  end do
  elapsed = omp_get_wtime() - t0

  print "(a,i0,a,i0,a,i0,a,f0.4,a,f0.2,a)", &
       "最終: N=", N, ", D=", D, ", epochs=", E, &
       ", loss=", loss, ", acc=", 100.0d0 * correct / N, "%"
  print "(a,f0.3,a)", "elapsed = ", elapsed, " sec"
end program regression
