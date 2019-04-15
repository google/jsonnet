// A benchmark for builtin sort

local reverse = std.reverse;
local sort = std.sort;

true
&& std.assertEqual(std.range(1, 500), sort(std.range(1, 500)))
&& std.assertEqual(std.range(1, 1000), sort(std.range(1, 1000)))
&& std.assertEqual(reverse(std.range(1, 1000)), sort(std.range(1, 1000), keyF=function(x) -x))
&& std.assertEqual(std.range(1, 1000), sort(reverse(std.range(1, 1000))))
&& std.assertEqual(std.makeArray(2000, function(i) std.floor((i + 2) / 2)), sort(std.range(1, 1000) + reverse(std.range(1, 1000))))
