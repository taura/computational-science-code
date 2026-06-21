! 本物の MNIST 手書き数字を, 学習済みの2層MLPで認識する (推論=forward)。
!   data/mnist_weights.txt : 学習済みの重み (784->128->10)
!   data/mnist_test.txt    : テスト画像 (28x28=784画素, 0..255) と正解ラベル
! 推論の中身は「行列ベクトル積 + 活性化(ReLU) + argmax」。各画像の推論は独立なので並列化できる。
program mnist_infer
  use omp_lib
  implicit none
  integer :: IN, HID, OUT, NT, IN2, i, hh, oo, k, best, lab
  integer(8) :: correct
  real(8) :: s, bestv, t0, elapsed
  real(8), allocatable :: W1(:,:), b1(:), W2(:,:), b2(:), X(:,:), hidv(:)
  integer, allocatable :: y(:), pix(:)

  ! --- 重みの読み込み ---
  open(10, file="data/mnist_weights.txt", status="old", action="read")
  read(10, *) IN, HID, OUT
  allocate(W1(IN,HID), b1(HID), W2(HID,OUT), b2(OUT))   ! W1(k,hh)=W1[hh][k] の並び
  read(10, *) W1        ! HID*IN 個 (hh ごとに IN 個ずつ) を列優先 W1(:,hh) に読み込む
  read(10, *) b1
  read(10, *) W2        ! OUT*HID 個
  read(10, *) b2
  close(10)

  ! --- テスト画像の読み込み (画素 0..255 -> 0..1) ---
  open(10, file="data/mnist_test.txt", status="old", action="read")
  read(10, *) NT, IN2
  allocate(X(IN,NT), y(NT), pix(IN))
  do i = 1, NT
     read(10, *) pix, lab
     X(:,i) = real(pix, 8) / 255.0d0
     y(i) = lab
  end do
  close(10)

  ! --- 推論 ---
  correct = 0
  t0 = omp_get_wtime()
  ! BEGIN ANSWER: 各画像の推論は独立。!$omp parallel do reduction(+:correct) で並列化せよ.
  !$omp parallel do private(hidv,s,best,bestv,hh,oo,k) reduction(+:correct)
  ! END ANSWER
  do i = 1, NT
     allocate(hidv(HID))
     do hh = 1, HID                       ! h = ReLU(W1 x + b1)
        s = b1(hh)
        do k = 1, IN
           s = s + W1(k,hh) * X(k,i)
        end do
        hidv(hh) = max(0.0d0, s)
     end do
     best = 1; bestv = -1d300              ! o = W2 h + b2, argmax
     do oo = 1, OUT
        s = b2(oo)
        do hh = 1, HID
           s = s + W2(hh,oo) * hidv(hh)
        end do
        if (s > bestv) then
           bestv = s; best = oo
        end if
     end do
     if (best - 1 == y(i)) correct = correct + 1   ! クラスは 0..9, best は 1..10
     deallocate(hidv)
  end do
  ! BEGIN ANSWER: 上で始めた parallel do 領域を閉じる (!$omp end parallel do).
  !$omp end parallel do
  ! END ANSWER
  elapsed = omp_get_wtime() - t0

  print "(a,i0,a,i0,a,f0.2,a)", &
       "MNIST テスト ", NT, " 枚: 正解 ", correct, " 枚, 正解率 = ", &
       100.0d0 * correct / NT, " %"
  print "(a,f0.3,a)", "elapsed = ", elapsed, " sec"
end program mnist_infer
