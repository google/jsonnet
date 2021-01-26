# taken from https://stackoverflow.com/a/49086560

# tested with the toolchain from ARM:
# gcc-arm-9.2-2019.12-mingw-w64-i686-arm-none-linux-gnueabihf.tar.xz
# found at
# https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-a/downloads

# see also:
# https://stackoverflow.com/questions/42371788/how-to-run-helloworld-on-arm
# https://dev.to/younup/cmake-on-stm32-the-beginning-3766

SET(CMAKE_SYSTEM_NAME Linux)
SET(CMAKE_SYSTEM_PROCESSOR arm)
SET(CMAKE_SYSTEM_VERSION 1)
set(CMAKE_CROSSCOMPILING TRUE)

find_program(CC_GCC arm-none-linux-gnueabihf-gcc REQUIRED)

set(CMAKE_FIND_ROOT_PATH /usr/arm-linux-gnueabihf)

# Cross compiler
SET(CMAKE_C_COMPILER   arm-none-linux-gnueabihf-gcc)
SET(CMAKE_CXX_COMPILER arm-none-linux-gnueabihf-g++)
set(CMAKE_LIBRARY_ARCHITECTURE arm-none-linux-gnueabihf)

# Search for programs in the build host directories
SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

# Libraries and headers in the target directories
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

set(THREADS_PTHREAD_ARG "0" CACHE STRING "Result from TRY_RUN" FORCE)

get_filename_component(TOOLCHAIN_DIR "${CC_GCC}" DIRECTORY)
get_filename_component(TOOLCHAIN_DIR "${TOOLCHAIN_DIR}" DIRECTORY)
set(TOOLCHAIN_SO_DIR "${TOOLCHAIN_DIR}/arm-none-linux-gnueabihf/libc/")

#/home/jpmag/local/arm/gcc-arm-9.2-2019.12-x86_64-arm-none-linux-gnueabihf
set(CMAKE_CROSSCOMPILING_EMULATOR qemu-arm -L ${TOOLCHAIN_SO_DIR})

return()


set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)
set(CMAKE_SYSTEM_VERSION 1)
set(CMAKE_CROSSCOMPILING 1)

set(CMAKE_C_COMPILER "arm-none-eabi-gcc")
set(CMAKE_CXX_COMPILER "arm-none-eabi-g++")


set(CMAKE_FIND_ROOT_PATH /usr/arm-none-eabi)
set(CMAKE_EXE_LINKER_FLAGS "--specs=nosys.specs" CACHE INTERNAL "")

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

set(COMPILER_FLAGS "-marm -mfpu=neon  -mfloat-abi=hard -mcpu=cortex-a9 -D_GNU_SOURCE")

message(STATUS)
message(STATUS)
message(STATUS)

if(NOT DEFINED CMAKE_C_FLAGS)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${COMPILER_FLAGS}" CACHE STRING "")
else()
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${COMPILER_FLAGS}")
endif()

message(STATUS)
message(STATUS)
message(STATUS)
message(STATUS)

if(NOT DEFINED CMAKE_CXX_FLAGS)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMPILER_FLAGS}" CACHE STRING "")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMPILER_FLAGS}")
endif()
