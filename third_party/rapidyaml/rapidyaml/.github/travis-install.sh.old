#!/usr/bin/env bash

set -e
set -x

# input environment variables:
# CXX_: the compiler version. eg, g++-9 or clang++-6.0

#-------------------------------------------------------------------------------

# add a gcc compiler
function addgcc()
{
    version=$1
    addpkg g++-$version
    addpkg g++-$version-multilib
}

# add a clang compiler
function addclang()
{
    version=$1
    case $version in
        # in 18.04, clang9 and later require PPAs
        9 | 10 ) addpkg clang-$version "deb http://apt.llvm.org/bionic/ llvm-toolchain-bionic-$version main" ;;
        *      ) addpkg clang-$version ;;
    esac
    addpkg g++-multilib  # this is required for 32 bit https://askubuntu.com/questions/1057341/unable-to-find-stl-headers-in-ubuntu-18-04
    addpkg clang-tidy-$version
}

# add a debian package to the list
function addpkg()
{
    pkgs=$1
    sourceslist=$2
    DPKG="$DPKG $pkgs"
    #echo "DPKG=$DPKG"
    addsrc "$sourceslist" "# for packages: $pkgs"
}

# add an apt source
function addsrc()
{
    sourceslist=$1
    comment=$2
    if [ ! -z "$sourceslist" ] ; then
        sudo bash -c "cat >> /etc/apt/sources.list <<EOF
$comment
$sourceslist
EOF"
        #cat /etc/apt/sources.list
    fi
}


#-------------------------------------------------------------------------------


case $CXX_ in
    g++-10     ) addgcc 10 ;;
    g++-9      ) addgcc 9  ;;
    g++-8      ) addgcc 8  ;;
    g++-7      ) addgcc 7  ;;
    g++-6      ) addgcc 6  ;;
    g++-5      ) addgcc 5  ;;
    g++-4.9    ) addgcc 4.9 ;;
    clang++-10 ) addclang 10  ;;
    clang++-9  ) addclang 9   ;;
    clang++-8  ) addclang 8   ;;
    clang++-7  ) addclang 7   ;;
    clang++-6.0) addclang 6.0 ;;
    clang++-5.0) addclang 5.0 ;;
    clang++-4.0) addclang 4.0 ;;
    clang++-3.9) addclang 3.9 ;;
    default)
        echo "unknown compiler: $CXX_"
        exit 1
        ;;
esac

if [ "$BT" == "Coverage" ] ; then
    addpkg lcov
    addpkg libffi-dev
    addpkg libssl-dev
fi

echo "additional packages: $DPKG"

wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key 2>/dev/null | sudo apt-key add -
wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | sudo apt-key add -

sudo -E apt-add-repository --yes --no-update 'deb https://apt.kitware.com/ubuntu/ bionic main'
sudo -E add-apt-repository --yes --no-update ppa:ubuntu-toolchain-r/test

sudo -E apt-get clean
sudo -E apt-get update

sudo -E apt-get install -y --force-yes \
     build-essential \
     cmake \
     valgrind \
     linux-libc-dev:i386 \
     libc6:i386 \
     libc6-dev:i386 \
     libc6-dbg:i386 \
     $DPKG \
     python3-pip \
     python3-setuptools

if [ "$BT" == "Coverage" ]; then
    sudo pip3 install \
         requests[security] \
         pyopenssl \
         ndg-httpsclient \
         pyasn1 \
         cpp-coveralls
fi

dpkg -s cmake
dpkg -L cmake
which cmake
cmake --version
$CXX_ --version
which $CXX_

echo "INSTALL COMPLETE: current directory: $(pwd)"
