std.parseYaml(|||
  foo: &foo foo
  bar: *foo
|||)
