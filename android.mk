# -------------------- VALUES TO CONFIGURE --------------------

include versions.mk

ifeq ($(V),)
HIDE=@
MAKE += --no-print-directory
endif
PWD	:= $(shell pwd)

# Path to Android NDK
# NDK version must match ndkVersion in app/build.gradle
NDK_PATH := /opt/Android/Sdk/ndk/$(shell grep ndkVersion app/android/app/build.gradle.kts | sed 's/[^0-9.]*//g')

# Android API level 26 == Android 8.0
API_LEVEL := 26

# Set default from following values: [armeabi-v7a, arm64-v8a, x86_64]
ANDROID_TARGET_ARCH := arm64-v8a

# -------------------- GENERATED VALUES --------------------

ifeq ($(ANDROID_TARGET_ARCH), armeabi-v7a)
	TARGET       := arm-linux-androideabi
	CLANG_TARGET := armv7a-linux-androideabi
	ARCH         := arm
	OPENSSL_ARCH := android-arm
	MARCH        := armv7-a
else
ifeq ($(ANDROID_TARGET_ARCH), arm64-v8a)
	TARGET       := aarch64-linux-android
	CLANG_TARGET := $(TARGET)
	ARCH         := arm
	OPENSSL_ARCH := android-arm64
	MARCH        := armv8-a
else
ifeq ($(ANDROID_TARGET_ARCH), x86_64)
	TARGET       := x86_64-linux-android
	CLANG_TARGET := $(TARGET)
	ARCH         := x86
	OPENSSL_ARCH := android-x86_64
	MARCH        := x86-64
else
	exit 1
endif
endif
endif

PLATFORM := android-$(API_LEVEL)

OS := $(shell uname -s | tr "[A-Z]" "[a-z]")
ifeq ($(OS),linux)
	HOST_OS   := linux-x86_64
endif
ifeq ($(OS),darwin)
	HOST_OS   := darwin-x86_64
endif

# Toolchain and sysroot
TOOLCHAIN				:= $(NDK_PATH)/toolchains/llvm/prebuilt/$(HOST_OS)
CMAKE_TOOLCHAIN_FILE	:= $(NDK_PATH)/build/cmake/android.toolchain.cmake
SYSROOT					:= $(TOOLCHAIN)/sysroot
PKG_CONFIG_LIBDIR		:= $(NDK_PATH)/prebuilt/$(HOST_OS)/lib/pkgconfig

# Toolchain tools
PATH	:= $(TOOLCHAIN)/bin:${PATH}
AR		:= llvm-ar
AS		:= $(CLANG_TARGET)$(API_LEVEL)-clang
CC		:= $(CLANG_TARGET)$(API_LEVEL)-clang
CXX		:= $(CLANG_TARGET)$(API_LEVEL)-clang++
LD		:= ld.lld
RANLIB	:= llvm-ranlib
STRIP	:= llvm-strip

CMAKE_ANDROID_FLAGS := \
	-DANDROID=ON \
	-DANDROID_PLATFORM=$(API_LEVEL) \
	-DCMAKE_SYSTEM_NAME=Android \
	-DCMAKE_SYSTEM_VERSION=$(API_LEVEL) \
	-DCMAKE_TOOLCHAIN_FILE=$(CMAKE_TOOLCHAIN_FILE) \
	-DANDROID_ABI=$(ANDROID_TARGET_ARCH) \
	-DCMAKE_ANDROID_ARCH_ABI=$(ANDROID_TARGET_ARCH) \
	-DCMAKE_SKIP_INSTALL_RPATH=ON \
	-DCMAKE_C_COMPILER=$(CC) \
	-DCMAKE_CXX_COMPILER=$(CXX) \
	-DCMAKE_POSITION_INDEPENDENT_CODE=ON \
	-DCMAKE_BUILD_TYPE=Release

THIRD_PARTY_ROOT := ${PWD}/third_party_android/${ANDROID_TARGET_ARCH}

default: all

.PHONY: third_party_dir
third_party_dir:
	mkdir -p ${THIRD_PARTY_ROOT}/include
	mkdir -p ${THIRD_PARTY_ROOT}/lib

.PHONY: openssl
openssl: third_party_dir
	$(HIDE)cd third_party_android && \
		rm -rf openssl && \
		wget ${OPENSSL_MIRROR}/openssl-${OPENSSL_VERSION}.tar.gz && \
		tar -xzf openssl-${OPENSSL_VERSION}.tar.gz && \
		cp -a openssl-${OPENSSL_VERSION} openssl
	@rm -f third_party/openssl-${OPENSSL_VERSION}.tar.gz
	$(HIDE)cd third_party_android/openssl && \
		ANDROID_NDK_ROOT=$(NDK_PATH) \
		./Configure $(OPENSSL_ARCH) no-shared no-tests \
		-U__ANDROID_API__ -D__ANDROID_API__=$(API_LEVEL) && \
		make -j build_libs && \
		cp *.a ${THIRD_PARTY_ROOT}/lib && \
		cp -a include/openssl ${THIRD_PARTY_ROOT}/include/

.PHONY: lmdb
lmdb: third_party_dir
	$(HIDE)cd third_party_android && \
		rm -rf lmdb && \
		git clone https://github.com/LMDB/lmdb && \
		cd lmdb/libraries/liblmdb && \
		make CC=$(CC) -j && \
		cp liblmdb.a ${THIRD_PARTY_ROOT}/lib/ && \
		cp lmdb.h ${THIRD_PARTY_ROOT}/include/

.PHONY: opus
opus: third_party_dir
	$(HIDE)cd third_party_android && \
		rm -rf opus && \
		wget ${OPUS_MIRROR}/opus-${OPUS_VERSION}.tar.gz && \
		tar -xzf opus-${OPUS_VERSION}.tar.gz && \
		mv opus-${OPUS_VERSION} opus
	$(HIDE)cd third_party_android/opus && \
		CC=$(CC) ./configure --host=${TARGET} --disable-shared --with-pic --enable-dred \
		--enable-deep-plc --enable-osce && \
		make -j && \
		cp .libs/libopus.a ${THIRD_PARTY_ROOT}/lib/ && \
		mkdir -p ${THIRD_PARTY_ROOT}/include/opus && \
		cp include/*.h ${THIRD_PARTY_ROOT}/include/opus/

all: openssl lmdb opus
