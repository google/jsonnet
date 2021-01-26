#!/usr/bin/env bash

set -x

# https://askubuntu.com/questions/735201/installing-clang-3-8-on-ubuntu-14-04-3
wget -O - http://llvm.org/apt/llvm-snapshot.gpg.key | sudo apt-key add -

done=$(grep C4STL /etc/apt/sources.list)
if [ -z "$done" ] ; then
    cat >> /etc/apt/sources.list <<EOF

# C4STL
# http://apt.llvm.org/
deb http://llvm.org/apt/trusty/ llvm-toolchain-trusty-3.7 main
#deb http://llvm.org/apt/trusty/ llvm-toolchain-trusty-3.8 main
deb http://llvm.org/apt/trusty/ llvm-toolchain-trusty-3.9 main
deb http://llvm.org/apt/trusty/ llvm-toolchain-trusty-4.0 main
#deb http://llvm.org/apt/trusty/ llvm-toolchain-trusty-5.0 main
EOF
fi

sudo -E apt-get install -y software-properties-common python-software-properties
sudo -E add-apt-repository -y ppa:ubuntu-toolchain-r/test
sudo -E add-apt-repository -y ppa:george-edison55/cmake-3.x
sudo -E apt-get -yq update

sudo -E apt-get install -yq --force-yes \
     build-essential \
     cmake \
     g++-5 \
     g++-5-multilib \
     g++-6 \
     g++-6-multilib \
     g++-7 \
     g++-7-multilib \
     clang-3.7 \
     clang-3.8 \
     clang-3.9 \
     clang-4.0

exit 0
