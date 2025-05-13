// Test values at boundary of safe integer range
local max_safe = 9007199254740991;  // 2^53 - 1
local min_safe = -9007199254740991;  // -(2^53 - 1)

std.assertEqual(max_safe & 1, 1) &&  // Check 2^53
std.assertEqual(min_safe & 1, 1) &&  // Check -2^53
std.assertEqual((max_safe - 1) & 1, 0) &&  // Check 2^53 - 1
std.assertEqual((min_safe + 1) & 1, 0) &&  // Check -2^53

std.assertEqual(~(max_safe - 1), min_safe) &&  // ~(2^53 - 1) == -2^53
std.assertEqual(~(min_safe + 1), max_safe - 2) &&  // ~(-2^53 + 1) == 2^53 - 2

// Test basic values
std.assertEqual(~0, -1) &&
std.assertEqual(~1, -2) &&
std.assertEqual(~(-1), 0) &&

// Test shift operations with large values at safe boundary
// (2^53 - 1) right shift by 4 bits
std.assertEqual(max_safe >> 4, 562949953421311) &&
// MAX_SAFE_INTEGER (2^53 - 1) right shift by 1 bit is (2^52 - 1)
std.assertEqual(max_safe >> 1, 4503599627370495) &&  // 2^52
// MIN_SAFE_INTEGER -(2^53 - 1) right shift by 1 bit is (-2^52)
std.assertEqual(min_safe >> 1, -4503599627370496) &&  // -2^52

// Cannot left shift 2^53 without potential overflow/loss of precision issues
// depending on the shift amount, but can shift smaller numbers up to it.
// (2^52-1) left shift by 1 bit (result is 2^53-2)
std.assertEqual(4503599627370495 << 1, max_safe - 1) &&
// (-(2^52-1)) left shift by 1 bit (result is -(2^53-2))
std.assertEqual((-4503599627370495) << 1, min_safe + 1) &&

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
