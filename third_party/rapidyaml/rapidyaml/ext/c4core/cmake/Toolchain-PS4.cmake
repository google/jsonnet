# Copyright 2017 Autodesk Inc. http://www.autodesk.com
#
# Licensed under the Apache License, Version 2.0 (the "License"); you
# may not use this file except in compliance with the License. You may
# obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
# implied. See the License for the specific language governing
# permissions and limitations under the License.

# This module is shared; use include blocker.
if( _PS4_TOOLCHAIN_ )
	return()
endif()
set(_PS4_TOOLCHAIN_ 1)

# PS4 SCE version requirement
set(REQUIRED_PS4_VERSION "4.000")

# Get PS4 SCE environment
if( EXISTS "$ENV{SCE_ROOT_DIR}" AND IS_DIRECTORY "$ENV{SCE_ROOT_DIR}" )
	string(REGEX REPLACE "\\\\" "/" PS4_ROOT $ENV{SCE_ROOT_DIR})
	string(REGEX REPLACE "//" "/" PS4_ROOT ${PS4_ROOT})
	if( EXISTS "$ENV{SCE_ORBIS_SDK_DIR}" AND IS_DIRECTORY "$ENV{SCE_ORBIS_SDK_DIR}" )
		string(REGEX REPLACE "\\\\" "/" PS4_SDK $ENV{SCE_ORBIS_SDK_DIR})
		string(REGEX REPLACE "//" "/" PS4_SDK ${PS4_SDK})
		get_filename_component(SCE_VERSION "${PS4_SDK}" NAME)
	endif()
endif()

# Report and check version if it exist
if( NOT "${SCE_VERSION}" STREQUAL "" )
	message(STATUS "PS4 SCE version found: ${SCE_VERSION}")
	if( NOT "${SCE_VERSION}" MATCHES "${REQUIRED_PS4_VERSION}+" )
		message(WARNING "Expected PS4 SCE version: ${REQUIRED_PS4_VERSION}")
		if( PLATFORM_TOOLCHAIN_ENVIRONMENT_ONLY )
			set(PS4_ROOT)
			set(PS4_SDK)
		endif()
	endif()
endif()

# If we only want the environment values, exit now
if( PLATFORM_TOOLCHAIN_ENVIRONMENT_ONLY )
	return()
endif()

# We are building PS4 platform, fail if PS4 SCE not found
if( NOT PS4_ROOT OR NOT PS4_SDK )
	message(FATAL_ERROR "Engine requires PS4 SCE SDK to be installed in order to build PS4 platform.")
endif()

# Tell CMake we are cross-compiling to PS4 (Orbis)
set(CMAKE_SYSTEM_NAME Orbis)
set(PS4 True)

# Set CMake system root search path
set(CMAKE_SYSROOT "${PS4_ROOT}")

# Set compilers to the ones found in PS4 SCE SDK directory
set(CMAKE_C_COMPILER "${PS4_SDK}/host_tools/bin/orbis-clang.exe")
set(CMAKE_CXX_COMPILER "${PS4_SDK}/host_tools/bin/orbis-clang++.exe")
set(CMAKE_ASM_COMPILER "${PS4_SDK}/host_tools/bin/orbis-as.exe")

# Only search the PS4 SCE SDK, not the remainder of the host file system
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
