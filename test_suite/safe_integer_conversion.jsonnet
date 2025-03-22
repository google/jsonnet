// Test values at boundary of safe integer range
std.assertEqual(~9007199254740991, -9007199254740992) &&  // ~MAX_SAFE_INTEGER
std.assertEqual(~(-9007199254740991), 9007199254740990) &&  // ~MIN_SAFE_INTEGER

// Test basic values
std.assertEqual(~0, -1) &&
std.assertEqual(~1, -2) &&
std.assertEqual(~(-1), 0) &&

// Test shift operations with large values at safe boundary
// MAX_SAFE_INTEGER (2^53-1) right shift by 4 bits
std.assertEqual(9007199254740991 >> 4, 562949953421311) &&
// MAX_SAFE_INTEGER (2^53-1) left shift by 1 bit (result is still precisely representable)
std.assertEqual(9007199254740991 << 1, 18014398509481982) &&
// (2^52-1) left shift by 1 bit (result is MAX_SAFE_INTEGER-1)
std.assertEqual(4503599627370495 << 1, 9007199254740990) &&

// Test larger values within safe range
std.assertEqual(~123456789, -123456790) &&
std.assertEqual(~(-987654321), 987654320) &&

// Test other bitwise operations
std.assertEqual(123 & 456, 72) &&
std.assertEqual(123 | 456, 507) &&
std.assertEqual(123 ^ 456, 435) &&
std.assertEqual(123 << 2, 492) &&
std.assertEqual(123 >> 2, 30) &&

true
