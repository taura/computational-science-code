program average
  real(8) :: a, b, m
  a = 3.0d0
  b = 4.0d0
  ! BEGIN ANSWER: a と b の平均を計算して m に代入せよ (右辺を埋める).
  m = (a + b) / 2.0d0
  ! END ANSWER
  print "(a,f0.6,a,f0.6,a,f0.6)", "average of ", a, " and ", b, " is ", m
end program average
