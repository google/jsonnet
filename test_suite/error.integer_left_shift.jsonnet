// Test that left-shifting a value that would exceed int64_t range throws an error
local large_value = 1 << 62;  // 2^62
large_value << 2  // Would be 2^64, exceeding signed 64-bit integer range
