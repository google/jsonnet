// Value just beyond MAX_SAFE_INTEGER (2^53 - 1)
local beyond_max = 9007199254740992;  // 2^53
beyond_max << 1  // Should throw error "numeric value outside safe integer range for bitwise operation"
