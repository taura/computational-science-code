program target_offload
  use omp_lib
  ! TODO: 下の print を !$omp target ... !$omp end target で囲み, デバイス(GPU)上で実行させよ.
  print "(a)", "hello from the device"
  ! TODO: 上で始めた target 領域を閉じる (!$omp end target).
end program target_offload
