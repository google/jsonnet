#ifndef _C4_EXT_FAST_FLOAT_HPP_
#define _C4_EXT_FAST_FLOAT_HPP_

#ifdef _MSC_VER
#   pragma warning(push)
#   pragma warning(disable: 4996) // snprintf/scanf: this function or variable may be unsafe
#elif defined(__clang__)
#   pragma clang diagnostic push
#elif defined(__GNUC__)
#   pragma GCC diagnostic push
#   pragma GCC diagnostic ignored "-Wuseless-cast"
#endif

#include "./fast_float/include/fast_float/fast_float.h"

#ifdef _MSC_VER
#   pragma warning(pop)
#elif defined(__clang__)
#   pragma clang diagnostic pop
#elif defined(__GNUC__)
#   pragma GCC diagnostic pop
#endif

#endif // _C4_EXT_FAST_FLOAT_HPP_
