local fibnext = {
  a: super.a + super.b,
  b: super.a,
};
local fib(n) =
  if n == 0 then
    { a: 1, b: 1 }
  else
    fib(n - 1) + fibnext;

fib(25)
