local d = import 'd.jsonnet';
local c = import 'c.jsonnet';
assert 42 == 42;  // further imports are not top-level
local b = import 'b.jsonnet';
local a = import 'a.jsonnet';
true
