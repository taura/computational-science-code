program average
  real(8) :: a, b, m
  a = 3.0d0
  b = 4.0d0
  ! TODO: a と b の平均を計算して m に代入せよ (右辺を埋める).
  m = a + b / 2
  print "(a,f0.6,a,f0.6,a,f0.6)", "average of ", a, " and ", b, " is ", m
end program average
