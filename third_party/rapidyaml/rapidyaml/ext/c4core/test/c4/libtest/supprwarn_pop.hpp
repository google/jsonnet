#ifndef _C4_SUPPRWARN_POP_HPP_
#define _C4_SUPPRWARN_POP_HPP_

#ifdef __clang__
#   pragma clang diagnostic pop
#elif defined(__GNUC__)
#   pragma GCC diagnostic pop
#elif defined(_MSC_VER)
#   pragma warning(pop)
#endif

#endif /* SUPPRWARN_POP_H */
