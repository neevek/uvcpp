#!/bin/bash

if [[ "$NDK" = "" ]]; then
  echo "\$NDK is empty"
  exit 1
fi

SRC_DIR=$(pwd)
API=21

mkdir -p build/android

cd build
rm -rf CMakeFiles CMakeCache.txt Makefile cmake_install deps

#ANDROID_TOOLCHAIN=$SRC_DIR/build/android/toolchain-$API
#if [[ ! -d $ANDROID_TOOLCHAIN ]]; then
  #$NDK/build/tools/make-standalone-toolchain.sh \
    #--toolchain=arm-linux-androideabi-4.9 \
    #--arch=arm \
    #--install-dir=$ANDROID_TOOLCHAIN \
    #--platform=android-$API \
    #--force
#fi
#export CC=$ANDROID_TOOLCHAIN/bin/arm-linux-androideabi-gcc #export CXX=$ANDROID_TOOLCHAIN/bin/arm-linux-androideabi-g++
#export LINK=$ANDROID_TOOLCHAIN/bin/arm-linux-androideabi-g++
#export AR=$ANDROID_TOOLCHAIN/bin/arm-linux-androideabi-ar
#export PLATFORM=android
#export CFLAGS="-D__ANDROID_API__=$API"


HOST_TAG=darwin-x86_64
export TOOLCHAIN=$NDK/toolchains/llvm/prebuilt/$HOST_TAG
export AR=$TOOLCHAIN/bin/aarch64-linux-android-ar
export AS=$TOOLCHAIN/bin/aarch64-linux-android-as
export CC=$TOOLCHAIN/bin/aarch64-linux-android21-clang
export CXX=$TOOLCHAIN/bin/aarch64-linux-android21-clang++
export LD=$TOOLCHAIN/bin/aarch64-linux-android-ld
export RANLIB=$TOOLCHAIN/bin/aarch64-linux-android-ranlib
export STRIP=$TOOLCHAIN/bin/aarch64-linux-android-strip
export PLATFORM=android
export LDFLAGS=""
ARCH=arm64-v8a

cmake \
  -DCMAKE_TOOLCHAIN_FILE=$NDK/build/cmake/android.toolchain.cmake \
  -DCMAKE_ANDROID_NDK=$NDK \
  -DCMAKE_SYSTEM_NAME=Android \
  -DCMAKE_SYSTEM_VERSION=$API \
  -DCMAKE_ANDROID_ARCH_ABI=$ARCH \
  -DCMAKE_ANDROID_STL_TYPE=c++_shared \
  -DCMAKE_CROSSCOMPILING=TRUE \
  -DANDROID_PLATFORM=android-21 \
  -DANDROID_ABI=$ARCH \
  ..

NPROCESSORS=$(getconf NPROCESSORS_ONLN 2>/dev/null || getconf _NPROCESSORS_ONLN 2>/dev/null)
PROCESSORS=${NPROCESSORS:-1}
make -j${PROCESSORS}
