program target_offload
  use omp_lib
  ! TODO: 下の print を !$omp target ... !$omp end target で囲み, デバイス(GPU)上で実行させよ.
  ! BEGIN ANSWER
  !$omp target
  ! END ANSWER
  print "(a)", "hello from the device"
  ! BEGIN ANSWER
  !$omp end target
  ! END ANSWER
end program target_offload
