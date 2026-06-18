program where_am_i
  use omp_lib
  print "(a,i0)", "on host: omp_is_initial_device() = ", omp_is_initial_device()
  ! TODO: 下の print を !$omp target ... !$omp end target で囲み, デバイス(GPU)上で実行させよ. (表示するだけなので map 節は不要)
  print "(a,i0)", "inside target: omp_is_initial_device() = ", omp_is_initial_device()
  ! TODO: 上で始めた target 領域を閉じる (!$omp end target).
end program where_am_i
