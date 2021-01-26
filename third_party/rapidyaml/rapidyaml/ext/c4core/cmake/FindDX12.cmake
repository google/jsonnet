# Attempt to find the D3D12 libraries
# Defines:
#
#  DX12_FOUND        - system has DX12
#  DX12_INCLUDE_PATH - path to the DX12 headers
#  DX12_LIBRARIES    - path to the DX12 libraries
#  DX12_LIB          - d3d12.lib

set(DX12_FOUND "NO")

if(WIN32)
    set(WIN10_SDK_DIR "C:/Program Files (x86)/Windows Kits/10")
    #set(WIN10_SDK_VERSION "10.0.10069.0")
    file(GLOB WIN10_SDK_VERSIONS
        LIST_DIRECTORIES TRUE
        RELATIVE "${WIN10_SDK_DIR}/Lib"
        "${WIN10_SDK_DIR}/Lib/*")
    list(SORT WIN10_SDK_VERSIONS)
    list(GET WIN10_SDK_VERSIONS -1 WIN10_SDK_VERSION)

    if(CMAKE_CL_64)
        set(w10ARCH x64)
    elseif(CMAKE_GENERATOR MATCHES "Visual Studio.*ARM" OR "${DXC_BUILD_ARCH}" STREQUAL "ARM")
        set(w10ARCH arm)
    elseif(CMAKE_GENERATOR MATCHES "Visual Studio.*ARM64" OR "${DXC_BUILD_ARCH}" STREQUAL "ARM64")
        set(w10ARCH arm64)
    else()
        set(w10ARCH x86)
    endif()

    # Look for the windows 8 sdk
    find_path(DX12_INC_DIR
        NAMES d3d12.h
        PATHS "${WIN10_SDK_DIR}/Include/${WIN10_SDK_VERSION}/um"
        DOC "Path to the d3d12.h file"
    )
    find_path(DXGI_INC_DIR
        NAMES dxgi1_4.h
        PATHS "${WIN10_SDK_DIR}/Include/${WIN10_SDK_VERSION}/shared"
        DOC "Path to the dxgi header file"
    )

    if(DX12_INC_DIR AND DXGI_INC_DIR)
        find_library(DX12_LIB
            NAMES d3d12
            PATHS "${WIN10_SDK_DIR}/Lib/${WIN10_SDK_VERSION}/um/${w10ARCH}"
            NO_DEFAULT_PATH
            DOC "Path to the d3d12.lib file"
        )
        find_library(DXGI_LIB
            NAMES dxgi
            PATHS "${WIN10_SDK_DIR}/Lib/${WIN10_SDK_VERSION}/um/${w10ARCH}"
            NO_DEFAULT_PATH
            DOC "Path to the dxgi.lib file"
        )
        if(DX12_LIB AND DXGI_LIB)
            set(DX12_FOUND "YES")
            set(DX12_LIBRARIES ${DX12_LIB} ${DXGI_LIB})
            mark_as_advanced(DX12_INC_DIR DX12_LIB)
            mark_as_advanced(DXGI_INC_DIR DXGI_LIB)
        endif()
    endif()
endif(WIN32)

if(DX12_FOUND)
    if(NOT DX12_FIND_QUIETLY)
        message(STATUS "DX12 headers found at ${DX12_INC_DIR}")
    endif()
else()
    if(DX12_FIND_REQUIRED)
        message(FATAL_ERROR "Could NOT find Direct3D12")
    endif()
    if(NOT DX12_FIND_QUIETLY)
        message(STATUS "Could NOT find Direct3D12")
    endif()
endif()
