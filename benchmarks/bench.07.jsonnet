local f2(f) = function(x) f(f(x));
local id(x) = x;

local slowId = std.makeArray(20, function(i) if i == 0 then id else f2(slowId[i - 1]));

slowId[15](42)
