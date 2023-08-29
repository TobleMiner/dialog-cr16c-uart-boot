#/usr/bin/env bash

set -x

TARGET=cr16-c-elf
PREFIX="$HOME/opt/cross"

GCC_VERSION=10.5.0
BINUTILS_VERSION=2.37
NEWLIB_VERSION=4.3.0.20230120

curl -O -L "https://ftp.gnu.org/gnu/gcc/gcc-$GCC_VERSION/gcc-$GCC_VERSION.tar.gz"
curl -O -L "https://ftp.gnu.org/gnu/binutils/binutils-$BINUTILS_VERSION.tar.gz"
curl -O -L "https://sourceware.org/pub/newlib/newlib-$NEWLIB_VERSION.tar.gz"

tar xzf gcc-$GCC_VERSION.tar.gz
tar xzf binutils-$BINUTILS_VERSION.tar.gz
tar xzf newlib-$NEWLIB_VERSION.tar.gz

mkdir build-{binutils,gcc,newlib}

pushd build-binutils
../binutils-$BINUTILS_VERSION/configure --target=$TARGET --prefix="$PREFIX" --with-sysroot --disable-nls --disable-werror
make -j
make install
popd

pushd build-gcc
../gcc-$GCC_VERSION/configure --target=$TARGET --prefix="$PREFIX" --disable-nls --enable-languages=c --without-headers
make -j all-gcc
make -j all-target-libgcc
make install-gcc
make install-target-libgcc
popd

pushd build-newlib
../newlib-$NEWLIB_VERSION/configure --target=$TARGET --prefix="$PREFIX"
make -j
make install
popd

echo done. run 'export PATH="$PATH:$PREFIX/bin"' now
