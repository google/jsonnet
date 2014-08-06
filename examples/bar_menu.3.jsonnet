// bar_menu.3.jsonnet
{
    foo: 3,     
    bar: 2 * self.foo,  // Multiplication.
    baz: "The value " + self.bar + " is "
         + (if self.bar > 5 then "large" else "small") + ".",
    array: [1, 2, 3] + [4],
    obj: {a: 1, b: 2} + {b: 3, c: 4},
}
