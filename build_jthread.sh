#!/bin/sh
# Written by Nxf <nick.xf@gmail.com>
# 
# Builds the JThread library for Android
export NDK_ROOT="/home/lor/tools/ndk"
SYS_ROOT="${NDK_ROOT}/platforms/android-L/arch-arm"
PREF="arm-linux-androideabi-"

OUT_DIR="`pwd`/android-build"
C_FLAGS="-lstdc++ -lsupc++ \
        -I${NDK_ROOT}/sources/cxx-stl/gnu-libstdc++/4.9/include \
        -I${NDK_ROOT}/sources/cxx-stl/gnu-libstdc++/4.9/libs/armeabi/include \
        -L${NDK_ROOT}/sources/cxx-stl/gnu-libstdc++/4.9/libs/armeabi"

#set -e

export CC="${NDK_ROOT}/toolchains/arm-linux-androideabi-4.9/prebuilt/linux-x86_64/bin/${PREF}gcc  --sysroot=${SYS_ROOT}"
export CXX="${NDK_ROOT}/toolchains/arm-linux-androideabi-4.9/prebuilt/linux-x86_64/bin/${PREF}g++  --sysroot=${SYS_ROOT}"
export LD="$${NDK_ROOT}/toolchains/arm-linux-androideabi-4.9/prebuilt/linux-x86_64/bin/{PREF}ld  --sysroot=${SYS_ROOT}"
export CPP="${NDK_ROOT}/toolchains/arm-linux-androideabi-4.9/prebuilt/linux-x86_64/bin/${PREF}cpp  --sysroot=${SYS_ROOT}"
export AS="${NDK_ROOT}/toolchains/arm-linux-androideabi-4.9/prebuilt/linux-x86_64/bin/${PREF}as"
export OBJCOPY="${NDK_ROOT}/toolchains/arm-linux-androideabi-4.9/prebuilt/linux-x86_64/bin/${PREF}objcopy"
export OBJDUMP="${NDK_ROOT}/toolchains/arm-linux-androideabi-4.9/prebuilt/linux-x86_64/bin/${PREF}objdump"
export STRIP="${NDK_ROOT}/toolchains/arm-linux-androideabi-4.9/prebuilt/linux-x86_64/bin/${PREF}strip"
export RANLIB="${NDK_ROOT}/toolchains/arm-linux-androideabi-4.9/prebuilt/linux-x86_64/bin/${PREF}ranlib"
export CCLD="${NDK_ROOT}/toolchains/arm-linux-androideabi-4.9/prebuilt/linux-x86_64/bin/${PREF}gcc  --sysroot=${SYS_ROOT}"
export AR="${NDK_ROOT}/toolchains/arm-linux-androideabi-4.9/prebuilt/linux-x86_64/bin/${PREF}ar"

cmake	\
		-DCMAKE_SYSTEM_NAME="Generic" \
        -DCMAKE_CXX_FLAGS="${C_FLAGS}"\
		-DCMAKE_FIND_ROOT_PATH="${SYS_ROOT}" \
		-DCMAKE_INSTALL_PREFIX="${OUT_DIR}" 
		
		

make
sudo make install

cd ${OUT_DIR}/lib && \
${AR} -x libjthread.a && \
${CXX} ${C_FLAGS} -shared -Wl,-soname,libjthread.so -o libjthread.so && \

exit 0