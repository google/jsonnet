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
if( _XB1_TOOLCHAIN_ )
	return()
endif()
set(_XB1_TOOLCHAIN_ 1)

# XB1 XDK version requirement
set(REQUIRED_XB1_TOOLCHAIN_VERSION "160305")

# Get XDK environment
if( EXISTS "$ENV{DurangoXDK}" AND IS_DIRECTORY "$ENV{DurangoXDK}" )
	string(REGEX REPLACE "\\\\" "/" XDK_ROOT $ENV{DurangoXDK})
	string(REGEX REPLACE "//" "/" XDK_ROOT ${XDK_ROOT})
endif()

# Fail if XDK not found
if( NOT XDK_ROOT )
	if( PLATFORM_TOOLCHAIN_ENVIRONMENT_ONLY )
		return()
	endif()
	message(FATAL_ERROR "Engine requires XB1 XDK to be installed in order to build XB1 platform.")
endif()

# Get toolchain version
get_filename_component(XDK_TOOLCHAIN_VERSION "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Wow6432Node\\Microsoft\\Durango XDK\\${REQUIRED_XB1_TOOLCHAIN_VERSION};EditionVersion]" NAME)

if( XDK_TOOLCHAIN_VERSION STREQUAL REQUIRED_XB1_TOOLCHAIN_VERSION )
	message(STATUS "Found required XDK toolchain version (${XDK_TOOLCHAIN_VERSION})")
else()
	get_filename_component(XDK_TOOLCHAIN_VERSION "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Wow6432Node\\Microsoft\\Durango XDK;Latest]" NAME)
	message(WARNING "Could not find required XDK toolchain version (${REQUIRED_XB1_TOOLCHAIN_VERSION}), using latest version instead (${XDK_TOOLCHAIN_VERSION})")
endif()

# If we only want the environment values, exit now
if( PLATFORM_TOOLCHAIN_ENVIRONMENT_ONLY )
	return()
endif()

# Find XDK compiler directory
if( CMAKE_GENERATOR STREQUAL "Visual Studio 11 2012" )
	set(XDK_COMPILER_DIR "${XDK_ROOT}/${XDK_TOOLCHAIN_VERSION}/Compilers/dev11.1")
elseif( CMAKE_GENERATOR STREQUAL "Visual Studio 14 2015" )
	get_filename_component(XDK_COMPILER_DIR "[HKEY_CURRENT_USER\\Software\\Microsoft\\VisualStudio\\14.0_Config\\Setup\\VC;ProductDir]" DIRECTORY)
	if( DEFINED XDK_COMPILER_DIR )
		string(REGEX REPLACE "\\\\" "/" XDK_COMPILER_DIR ${XDK_COMPILER_DIR})
		string(REGEX REPLACE "//" "/" XDK_COMPILER_DIR ${XDK_COMPILER_DIR})
	endif()
	if( NOT XDK_COMPILER_DIR )
		message(FATAL_ERROR "Can't find Visual Studio 2015 installation path.")
	endif()
else()
	message(FATAL_ERROR "Unsupported Visual Studio version!")
endif()

# Tell CMake we are cross-compiling to XBoxOne (Durango)
set(CMAKE_SYSTEM_NAME Durango)
set(XBOXONE True)

# Set CMake system root search path
set(CMAKE_SYSROOT "${XDK_COMPILER_DIR}")

# Set the compilers to the ones found in XboxOne XDK directory
set(CMAKE_C_COMPILER "${XDK_COMPILER_DIR}/vc/bin/amd64/cl.exe")
set(CMAKE_CXX_COMPILER "${XDK_COMPILER_DIR}/vc/bin/amd64/cl.exe")
set(CMAKE_ASM_COMPILER "${XDK_COMPILER_DIR}/vc/bin/amd64/ml64.exe")

# Force compilers to skip detecting compiler ABI info and compile features
set(CMAKE_C_COMPILER_FORCED True)
set(CMAKE_CXX_COMPILER_FORCED True)
set(CMAKE_ASM_COMPILER_FORCED True)

# Only search the XBoxOne XDK, not the remainder of the host file system
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# Global variables
set(XBOXONE_SDK_REFERENCES "Xbox Services API, Version=8.0;Xbox GameChat API, Version=8.0")
