# create a script that applies a patch (it's different in windows)

# to generate a patch:
# subversion: svn diff --patch-compatible > path/to/the/patch.diff


function(apply_patch patch where mark)
    if(NOT EXISTS "${mark}")
        create_patch_cmd(patch_cmd)
        file(TO_NATIVE_PATH ${patch} patch)
        file(TO_NATIVE_PATH ${where} where)
        file(TO_NATIVE_PATH ${mark} mark)
        message(STATUS "patching: ${patch_cmd} ${where} ${patch}  ${mark}")
        execute_process(COMMAND "${patch_cmd}" "${where}" "${patch}" "${mark}"
            RESULT_VARIABLE status)
        if(NOT status STREQUAL "0")
            message(FATAL_ERROR "could not apply patch: ${patch} ---> ${where}")
        endif()
    endif()
endfunction()


function(create_patch_cmd filename_output)
    if(WIN32)
        set(filename ${CMAKE_BINARY_DIR}/apply_patch.bat)
        file(WRITE ${filename} "
echo on
set srcdir=%1
set patch=%2
set mark=%3
set prev=%cd%
set stat=0
if not exist %mark% (
    if not exist %patch% (
        exit /b 1
    )
    if not exist %srcdir% (
        exit /b 1
    )
    cd %srcdir%
    patch -p0 < %patch%
    set stat=%ERRORLEVEL%
    cd %prev%
    echo done > %mark%
)
exit /b %stat%
")
    else()
        set(filename ${CMAKE_BINARY_DIR}/apply_patch.sh)
        file(WRITE ${filename} "#!/bin/sh -x
set -e
srcdir=$1
patch=$2
mark=$3
if [ ! -f $mark ] ; then
    if [ ! -f $patch ] ; then
        echo \"ERROR: patch not found: $patch\"
        exit 1
    fi
    cd $srcdir || exit 1
    patch -p0 < $patch || exit 1
    cd -
    echo done > $mark
fi
exit 0
")
    endif()
    set(${filename_output} ${filename} PARENT_SCOPE)
endfunction()
