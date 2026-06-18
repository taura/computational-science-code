program target_offload
  use omp_lib
  ! BEGIN ANSWER: 下の print を !$omp target ... !$omp end target で囲み, デバイス(GPU)上で実行させよ.
  !$omp target
  ! END ANSWER
  print "(a)", "hello from the device"
  ! BEGIN ANSWER: 上で始めた target 領域を閉じる (!$omp end target).
  !$omp end target
  ! END ANSWER
end program target_offload
