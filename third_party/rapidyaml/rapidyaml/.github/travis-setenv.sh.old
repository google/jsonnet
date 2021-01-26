#!/usr/bin/env bash

set -e
set -x

NUM_JOBS=2
PROJ_DIR=$(pwd)
PROJ_PFX=RYML_

pwd
export CC_=$(echo "$CXX_" | sed 's:clang++:clang:g' | sed 's:g++:gcc:g')
$CXX_ --version
$CC_ --version
cmake --version

# add cmake flags, without project prefix
function addcmkflags()
{
    for f in $* ; do
        CMFLAGS="$CMFLAGS ${f}"
    done
}
# add cmake flags, with project prefix
function addprojflags()
{
    for f in $* ; do
        CMFLAGS="$CMFLAGS -D${PROJ_PFX}${f}"
    done
}

addcmkflags -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
addprojflags DEV=ON

case "$LINT" in
    all       ) addprojflags LINT=ON LINT_TESTS=ON LINT_CLANG_TIDY=ON  LINT_PVS_STUDIO=ON ;;
    clang-tidy) addprojflags LINT=ON LINT_TESTS=ON LINT_CLANG_TIDY=ON  LINT_PVS_STUDIO=OFF ;;
    pvs-studio) addprojflags LINT=ON LINT_TESTS=ON LINT_CLANG_TIDY=OFF LINT_PVS_STUDIO=ON ;;
    *         ) addprojflags LINT=OFF ;;
esac

case "$SAN" in
    ALL) addprojflags SANITIZE=ON ;;
    A  ) addprojflags SANITIZE=ON ASAN=ON  TSAN=OFF MSAN=OFF UBSAN=OFF ;;
    T  ) addprojflags SANITIZE=ON ASAN=OFF TSAN=ON  MSAN=OFF UBSAN=OFF ;;
    M  ) addprojflags SANITIZE=ON ASAN=OFF TSAN=OFF MSAN=ON  UBSAN=OFF ;;
    UB ) addprojflags SANITIZE=ON ASAN=OFF TSAN=OFF MSAN=OFF UBSAN=ON ;;
    *  ) addprojflags SANITIZE=OFF ;;
esac

case "$SAN_ONLY" in
    ON) addprojflags SANITIZE_ONLY=ON ;;
    * ) addprojflags SANITIZE_ONLY=OFF ;;
esac

case "$VG" in
    ON) addprojflags VALGRIND=ON VALGRIND_SGCHECK=OFF ;; # FIXME SGCHECK should be ON
    * ) addprojflags VALGRIND=OFF VALGRIND_SGCHECK=OFF ;;
esac

case "$BM" in
    ON) addprojflags BUILD_BENCHMARKS=ON ;;
    * ) addprojflags BUILD_BENCHMARKS=OFF ;;
esac

if [ "$STD" != "" ] ; then
    addcmkflags -DC4_CXX_STANDARD=$STD
    addprojflags CXX_STANDARD=$STD
fi

if [ "$BT" == "Coverage" ] ; then
    # the coverage repo tokens can be set in the travis environment:
    # export CODECOV_TOKEN=.......
    # export COVERALLS_REPO_TOKEN=.......
    addprojflags COVERAGE_CODECOV=ON COVERAGE_CODECOV_SILENT=ON
    addprojflags COVERAGE_COVERALLS=ON COVERAGE_COVERALLS_SILENT=ON
fi

echo "building with additional cmake flags: $CMFLAGS"

export C4_EXTERN_DIR=`pwd`/build/extern
mkdir -p $C4_EXTERN_DIR


function ryml_cfg_test()
{
    bits=$1
    linktype=$2
    #
    build_dir=`pwd`/build/$bits-$linktype
    install_dir=`pwd`/install/$bits-$linktype
    mkdir -p $build_dir
    mkdir -p $install_dir
    #
    case "$linktype" in
        static) linktype="-DBUILD_SHARED_LIBS=OFF" ;;
        dynamic) linktype="-DBUILD_SHARED_LIBS=ON" ;;
    esac
    cmake -S $PROJ_DIR -B $build_dir \
          -DCMAKE_C_COMPILER=$CC_ -DCMAKE_C_FLAGS="-std=c99 -m$bits" \
          -DCMAKE_CXX_COMPILER=$CXX_ -DCMAKE_CXX_FLAGS="-m$bits" \
          -DCMAKE_INSTALL_PREFIX="$install_dir" \
          -DCMAKE_BUILD_TYPE=$BT \
          $CMFLAGS \
          $linktype
    cmake --build $build_dir --target help | sed 1d | sort
}

function ryml_run_test()
{
    bits=$1
    linktype=$2
    build_dir=`pwd`/build/$bits-$linktype
    export CTEST_OUTPUT_ON_FAILURE=1
    cmake --build $build_dir --parallel $NUM_JOBS --target test
}

function ryml_submit_coverage()
{
    if [ "$BT" == "Coverage" ] ; then
        bits=$1
        linktype=$2
        coverage_service=$3
        build_dir=`pwd`/build/$bits-$linktype
        echo "Submitting coverage data: $build_dir --> $coverage_service"
        cmake --build $build_dir --parallel $NUM_JOBS --target ryml-coverage-submit-$coverage_service
    fi
}
