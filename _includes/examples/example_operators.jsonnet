// example_operators.jsonnet
{
    foo: [1, 2, 3],
    bar: [x * x for x in self.foo if x >= 2],
    baz: { ["field" + x]: x for x in self.foo },
    obj: { ["foo" + "bar"]: 3 },
}
