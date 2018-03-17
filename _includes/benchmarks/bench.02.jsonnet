local Fib = {
  n: 1,
  local outer = self,
  r: if self.n <= 1 then 1 else (Fib { n: outer.n - 1 }).r + (Fib { n: outer.n - 2 }).r,
};

(Fib { n: 25 }).r
