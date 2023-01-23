#!/bin/bash -x

maker=$1

if [ "${maker}" = "cmake" ]; then
    rm -rf build
    mkdir -p build
fi

mydir=$(dirname $0)
source ${mydir}/setup_env.sh

print "======================================== Environment"
env

print "======================================== Setup build"
export color=no
rm -rf ${top}/install
if [ "${maker}" = "make" ]; then
    make distclean
    make config CXXFLAGS="-Werror" prefix=${top}/install \
         || exit 10
fi
if [ "${maker}" = "cmake" ]; then
    cmake -Dcolor=no -DCMAKE_CXX_FLAGS="-Werror" \
          -DCMAKE_INSTALL_PREFIX=${top}/install \
          .. \
          || exit 12
fi

print "======================================== Finished configure"
exit 0
