module func_mod
contains
  ! 2数の平均を返す関数 (引数2つ, 返り値1つ)
  function average(a, b) result(m)
    real(8), intent(in) :: a, b
    real(8) :: m
    m = (a + b) / 2.0d0
  end function average

  ! n! を計算する関数
  function factorial(n) result(p)
    integer, intent(in) :: n
    integer(8) :: p
    integer :: i
    p = 1
    do i = 2, n
       p = p * i
    end do
  end function factorial
end module func_mod

program func
  use func_mod
  print "(a,f0.6)", "average(3.0, 4.0) = ", average(3.0d0, 4.0d0)
  print "(a,i0)",   "factorial(5)      = ", factorial(5)
end program func
