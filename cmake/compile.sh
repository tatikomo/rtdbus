#!/bin/sh
#
#  compile script for cmake
# 

# make cmake happy 
export TEMP=/tmp

HOST_CC=gcc; export HOST_CC;

if test -d /usr/local/lib/pkgconfig; then 
    PKG_CONFIG_PATH="/usr/local/lib/pkgconfig:/usr/local/share/pkgconfig"; export PKG_CONFIG_PATH;
fi 

#    -DCMAKE_SYSTEM_NAME="Platform/Solaris" \
#cmake --trace \
cmake \
    -DCMAKE_VERBOSE_MAKEFILE=ON \
    -DCMAKE_INSTALL_PREFIX:PATH=/usr/local \
    -DCMAKE_INSTALL_LIBDIR:PATH=/usr/local/lib \
    -DINCLUDE_INSTALL_DIR:PATH=/usr/local/include \
    -DLIB_INSTALL_DIR:PATH=/usr/local/lib \
    -DSYSCONF_INSTALL_DIR:PATH=/usr/local/etc \
    -DSHARE_INSTALL_PREFIX:PATH=/usr/local/share \
    -DBUILD_SHARED_LIBS:BOOL=ON \
    -DCMAKE_C_COMPILER="/usr/local/bin/gcc" \
    -DCMAKE_CXX_COMPILER="/usr/local/bin/g++" \
    -DCMAKE_FIND_ROOT_PATH="/usr/local" \
    $*

