#ifndef _C4_CPU_HPP_
#define _C4_CPU_HPP_

/** @file cpu.hpp Provides processor information macros
 * @ingroup basic_headers */

// some of this is adapted from Qt's processor detection code:
// see http://code.qt.io/cgit/qt/qtbase.git/tree/src/corelib/global/qprocessordetection.h
// see also https://sourceforge.net/p/predef/wiki/Architectures/
// see also https://sourceforge.net/p/predef/wiki/Endianness/
// see also https://github.com/googlesamples/android-ndk/blob/android-mk/hello-jni/jni/hello-jni.c

#ifdef __ORDER_LITTLE_ENDIAN__
#   define _C4EL __ORDER_LITTLE_ENDIAN__
#else
#   define _C4EL 1234
#endif
#ifdef __ORDER_BIG_ENDIAN__
#   define _C4EB __ORDER_BIG_ENDIAN__
#else
#   define _C4EB 4321
#endif

#if defined(__x86_64) || defined(__x86_64__) || defined(__amd64) || defined(_M_X64)
#   define C4_CPU_X86_64
#   define C4_WORDSIZE 8
#   define C4_BYTE_ORDER _C4EL

#elif defined(__i386) || defined(__i386__) || defined(_M_IX86)
#   define C4_CPU_X86
#   define C4_WORDSIZE 4
#   define C4_BYTE_ORDER _C4EL

#elif defined(__ia64) || defined(__ia64__) || defined(_M_IA64)
#   define C4_CPU_IA64
#   define C4_WORDSIZE 8
// itanium is bi-endian - check byte order below

#elif defined(__arm__) || defined(_M_ARM) \
    || defined(__TARGET_ARCH_ARM) || defined(__aarch64__) || defined(_M_ARM64)
#   if defined(__aarch64__) || defined(_M_ARM64)
#       define C4_CPU_ARM64
#       define C4_CPU_ARMV8
#       define C4_WORDSIZE 8
#   else
#       define C4_CPU_ARM
#       define C4_WORDSIZE 4
#       if defined(__ARM_ARCH_8__) || (defined(__TARGET_ARCH_ARM) && __TARGET_ARCH_ARM >= 8)
#           define C4_CPU_ARMV8
#       elif defined(__ARM_ARCH_7__) || defined(_ARM_ARCH_7)      \
        || defined(__ARM_ARCH_7A__) || defined(__ARM_ARCH_7R__) \
        || defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7S__) \
        || (defined(__TARGET_ARCH_ARM) && __TARGET_ARCH_ARM >= 7)
#            define C4_CPU_ARMV7
#       elif defined(__ARM_ARCH_6__) || defined(__ARM_ARCH_6J__) \
        || defined(__ARM_ARCH_6T2__) || defined(__ARM_ARCH_6Z__) \
        || defined(__ARM_ARCH_6K__)  || defined(__ARM_ARCH_6ZK__) \
        || defined(__ARM_ARCH_6M__) \
        || (defined(__TARGET_ARCH_ARM) && __TARGET_ARCH_ARM >= 6)
#           define C4_CPU_ARMV6
#       elif defined(__ARM_ARCH_5TEJ__) \
        || (defined(__TARGET_ARCH_ARM) && __TARGET_ARCH_ARM >= 5)
#           define C4_CPU_ARMV5
#       elif defined(__ARM_ARCH_4T__) \
        || (defined(__TARGET_ARCH_ARM) && __TARGET_ARCH_ARM >= 5)
#           define C4_CPU_ARMV4
#       else
#           error "unknown CPU architecture: ARM"
#       endif
#   endif
#   ifdef __ARMEL__
#       define C4_BYTE_ORDER _C4EL
#   elif defined(__ARMEB__)
#       define C4_BYTE_ORDER _C4EB
#   endif

#elif defined(__ppc__) || defined(__ppc) || defined(__powerpc__)       \
    || defined(_ARCH_COM) || defined(_ARCH_PWR) || defined(_ARCH_PPC)  \
    || defined(_M_MPPC) || defined(_M_PPC)
#   if defined(__ppc64__) || defined(__powerpc64__) || defined(__64BIT__)
#       define C4_CPU_PPC64
#       define C4_WORDSIZE 8
#   else
#       define C4_CPU_PPC
#       define C4_WORDSIZE 4
#   endif
#elif defined(SWIG)
#else
#   error "unknown CPU architecture"
#endif

#define C4_LITTLE_ENDIAN (C4_BYTE_ORDER == _C4EL)
#define C4_BIG_ENDIAN (C4_BYTE_ORDER == _C4EB)

#endif /* _C4_CPU_HPP_ */
