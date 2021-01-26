#!/usr/bin/env bash

set -e
set -x

# input environment variables:
# OS: the operating system
# CXX_: the compiler version. eg, g++-9 or clang++-6.0
# BT: the build type
# VG: whether to install valgrind
# GITHUB_WORKFLOW: when run from github
# API: whether to install swig
# CMANY: whether to install cmany


#-------------------------------------------------------------------------------

function c4_install_test_requirements()
{
    # this is only for ubuntu ------------------
    os=$1
    case "$os" in
        ubuntu*) ;;
        macos*)
            if [ "$CMANY" == "ON" ] ; then
                sudo pip3 install cmany
            fi
            return 0
            ;;
        win*)
            if [ "$CMANY" == "ON" ] ; then
                pip install cmany
            fi
            if [ "$API" == "ON" ] ; then
                choco install swig
                which swig
            fi
            return 0
            ;;
        *)
            return 0
            ;;
    esac

    # gather all the requirements ------------------

    APT_PKG=""
    PIP_PKG=""

    if [ "$GITHUB_WORKFLOW" != "" ] ; then
        sudo dpkg --add-architecture i386
    else
        # travis requires build-essential + cmake
        _add_apt build-essential
        _add_apt cmake
    fi

    _add_apt linux-libc-dev:i386
    _add_apt libc6:i386
    _add_apt libc6-dev:i386
    _add_apt libc6-dbg:i386

    _c4_gather_compilers "$CXX_"

    _add_apt python3-setuptools
    _add_apt python3-pip

    #_add_apt iwyu
    #_add_apt cppcheck
    #_add_pip cpplint
    # oclint?
    if [ "$VG" == "ON" ] ; then
        _add_apt valgrind
    fi

    if [ "$BT" == "Coverage" ]; then
        _add_apt lcov
        _add_apt libffi-dev
        _add_apt libssl-dev
        _add_pip requests[security]
        _add_pip pyopenssl
        _add_pip ndg-httpsclient
        _add_pip pyasn1
        _add_pip cpp-coveralls
    fi

    if [ "$CMANY" != "" ] ; then
        _add_pip cmany
    fi

    wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | sudo apt-key add -
    sudo -E apt-add-repository --yes 'deb https://apt.kitware.com/ubuntu/ bionic main'
    sudo -E add-apt-repository --yes ppa:ubuntu-toolchain-r/test

    echo "apt packages: $APT_PKG"
    echo "pip packages: $PIP_PKG"

    # now install the requirements ------------------

    if [ "$APT_PKG" != "" ] ; then
        #sudo -E apt-get clean
        sudo -E apt-get update
        sudo -E apt-get install -y --force-yes $APT_PKG
    fi

    if [ "$PIP_PKG" != "" ]; then
        sudo pip3 install $PIP_PKG
    fi

    echo 'INSTALL COMPLETE!'
}


#-------------------------------------------------------------------------------

function _c4_gather_compilers()
{
    cxx=$1
    case $cxx in
        g++-10     ) _c4_addgcc 10 ;;
        g++-9      ) _c4_addgcc 9  ;;
        g++-8      ) _c4_addgcc 8  ;;
        g++-7      ) _c4_addgcc 7  ;;
        g++-6      ) _c4_addgcc 6  ;;
        g++-5      ) _c4_addgcc 5  ;;
        g++-4.9    ) _c4_addgcc 4.9 ;;
        clang++-10 ) _c4_addclang 10  ;;
        clang++-9  ) _c4_addclang 9   ;;
        clang++-8  ) _c4_addclang 8   ;;
        clang++-7  ) _c4_addclang 7   ;;
        clang++-6.0) _c4_addclang 6.0 ;;
        clang++-5.0) _c4_addclang 5.0 ;;
        clang++-4.0) _c4_addclang 4.0 ;;
        clang++-3.9) _c4_addclang 3.9 ;;
        all)
            all="g++-10 g++-9 g++-8 g++-7 g++-6 g++-5 g++-4.9 clang++-10 clang++-9 clang++-8 clang++-7 clang++-6.0 clang++-5.0 clang++-4.0 clang++-3.9"
            echo "installing all compilers: $all"
            for cxx in $all ; do
                _c4_gather_compilers $cxx
            done
            ;;
        "")
            # use default compiler
            ;;
        *)
            echo "unknown compiler: $cxx"
            exit 1
            ;;
    esac
}

# add a gcc compiler
function _c4_addgcc()
{
    version=$1
    _add_apt g++-$version
    _add_apt g++-$version-multilib
}

# add a clang compiler
function _c4_addclang()
{
    version=$1
    case $version in
        # in 18.04, clang9 and later require PPAs
        9 | 10 ) _add_apt clang-$version "deb http://apt.llvm.org/bionic/ llvm-toolchain-bionic-$version main" ;;
        *      ) _add_apt clang-$version ;;
    esac
    _add_apt g++-multilib  # this is required for 32 bit https://askubuntu.com/questions/1057341/unable-to-find-stl-headers-in-ubuntu-18-04
    _add_apt clang-tidy-$version
    wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key 2>/dev/null | sudo apt-key add -
}


#-------------------------------------------------------------------------------

# add a pip package to the list
function _add_pip()
{
    pkgs=$*
    PIP_PKG="$PIP_PKG $pkgs"
    echo "adding to pip packages: $pkgs"
}

# add a debian package to the list
function _add_apt()
{
    pkgs=$1
    sourceslist=$2
    APT_PKG="$APT_PKG $pkgs"
    echo "adding to apt packages: $pkgs"
    #echo "APT_PKG=$APT_PKG"
    _add_src "$sourceslist" "# for packages: $pkgs"
}

# add an apt source
function _add_src()
{
    sourceslist=$1
    comment=$2
    if [ ! -z "$sourceslist" ] ; then
        echo "adding apt source: $sourceslist"
        sudo bash -c "cat >> /etc/apt/sources.list <<EOF
$comment
$sourceslist
EOF"
        #cat /etc/apt/sources.list
    fi
}
