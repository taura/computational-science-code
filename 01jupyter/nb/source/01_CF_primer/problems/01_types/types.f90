program types
  real(8) :: c = 36.5d0   ! 摂氏
  real(8) :: f            ! 華氏 (これを求める)
  ! TODO: 摂氏 c を華氏 f に変換する式を書け (f = c * 9/5 + 32). 整数の割り算に注意.
  ! BEGIN ANSWER
  f = c * 9.0d0 / 5.0d0 + 32.0d0
  ! END ANSWER
  print "(f0.1,a,f0.1,a)", c, " C = ", f, " F"
end program types
