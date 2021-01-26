SET(CMAKE_SYSTEM_NAME Linux)
SET(CMAKE_SYSTEM_PROCESSOR arm)
SET(CMAKE_SYSTEM_VERSION 1)
set(CMAKE_CROSSCOMPILING TRUE)

find_program(CC_GCC arm-linux-gnueabihf-gcc REQUIRED)

set(CMAKE_FIND_ROOT_PATH /usr/arm-gnueabihf)

# Cross compiler
SET(CMAKE_C_COMPILER   arm-linux-gnueabihf-gcc)
SET(CMAKE_CXX_COMPILER arm-linux-gnueabihf-g++)
set(CMAKE_LIBRARY_ARCHITECTURE arm-linux-gnueabihf)

# Search for programs in the build host directories
SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

# Libraries and headers in the target directories
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

set(THREADS_PTHREAD_ARG "0" CACHE STRING "Result from TRY_RUN" FORCE)

get_filename_component(TOOLCHAIN_DIR "${CC_GCC}" DIRECTORY)
get_filename_component(TOOLCHAIN_DIR "${TOOLCHAIN_DIR}" DIRECTORY)
set(TOOLCHAIN_SO_DIR "${TOOLCHAIN_DIR}/arm-linux-gnueabihf/")
#/home/jpmag/local/arm/gcc-arm-9.2-2019.12-x86_64-arm-none-linux-gnueabihf
set(CMAKE_CROSSCOMPILING_EMULATOR qemu-arm -L ${TOOLCHAIN_SO_DIR})
