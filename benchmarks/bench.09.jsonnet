// This string must be longer than max stack frames
local veryLongString = std.join('', std.repeat(['e'], 510));

std.assertEqual(std.stripChars(veryLongString + 'ok' + veryLongString, 'e'), 'ok') &&
std.assertEqual(std.lstripChars(veryLongString + 'ok', 'e'), 'ok') &&
std.assertEqual(std.rstripChars('ok' + veryLongString, 'e'), 'ok') &&

true
