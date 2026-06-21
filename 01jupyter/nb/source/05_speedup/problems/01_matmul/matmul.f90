program matmul_speedup
  use omp_lib
  ! 密行列積 C = A * B (いずれも n x n).
  ! 行列は 1 次元配列に格納する (A(i*n+j+1) が A の (i,j) 要素, i,j は 0 始まり).
  ! 浮動小数点演算は乗算 + 加算が n 回 / 要素なので, 全体で 2 n^3 flops.
  character(len=32) :: arg
  integer(8) :: n, i, j, k
  real(8), allocatable :: A(:), B(:), C(:)
  real(8) :: s, t0, t1, dt, flops, checksum, expected
  n = 1000
  if (command_argument_count() >= 1) then
     call get_command_argument(1, arg); read (arg, *) n
  end if
  allocate(A(n * n), B(n * n), C(n * n))
  ! A も B も全要素 1.0 に初期化 -> C(i,j) = n になるはず (検算しやすい)
  A = 1.0d0; B = 1.0d0; C = 0.0d0
  print "(a,i0)", "n = ", n
  ! 計測開始
  t0 = omp_get_wtime()
  ! 計算本体: 3 重ループ. C[i][j] += A[i][k] * B[k][j]
  ! TODO: いちばん外側の i ループを !$omp parallel do ... !$omp end parallel do で囲み, 行ごとに並列化せよ.
  ! BEGIN ANSWER
  !$omp parallel do private(j, k, s)
  ! END ANSWER
  do i = 0, n - 1
     do j = 0, n - 1
        s = 0.0d0
        do k = 0, n - 1
           s = s + A(i * n + k + 1) * B(k * n + j + 1)
        end do
        C(i * n + j + 1) = s
     end do
  end do
  ! BEGIN ANSWER
  !$omp end parallel do
  ! END ANSWER
  ! 計測終了
  t1 = omp_get_wtime()
  dt = t1 - t0                  ! sec
  ! 検算: 全要素が n のはずなので, 総和は n * n * n
  checksum = sum(C)
  expected = real(n, 8) * real(n, 8) * real(n, 8)
  if (checksum == expected) then
     print "(a,f0.0,a,f0.0,a)", "checksum   : ", checksum, " (expected ", expected, ") -> OK"
  else
     print "(a,f0.0,a,f0.0,a)", "checksum   : ", checksum, " (expected ", expected, ") -> NG"
  end if
  flops = 2.d0 * real(n, 8) * real(n, 8) * real(n, 8)
  print "(a,f7.3,a)", "elapsed    : ", dt, "  sec"
  print "(a,es9.2)",  "flops      : ", flops
  print "(f7.3,a)", flops / dt * 1e-9, " GFLOPS"
end program matmul_speedup
