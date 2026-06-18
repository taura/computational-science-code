program where_am_i
  use omp_lib
  print "(a,i0)", "on host: omp_is_initial_device() = ", omp_is_initial_device()
  ! BEGIN ANSWER: 下の print を !$omp target ... !$omp end target で囲み, デバイス(GPU)上で実行させよ. (表示するだけなので map 節は不要)
  !$omp target
  ! END ANSWER
  print "(a,i0)", "inside target: omp_is_initial_device() = ", omp_is_initial_device()
  ! BEGIN ANSWER: 上で始めた target 領域を閉じる (!$omp end target).
  !$omp end target
  ! END ANSWER
end program where_am_i
