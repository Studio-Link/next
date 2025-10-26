#
# Makefile
#
# Copyright (C) 2023 Studio.Link Sebastian Reimers
# Variables (make CC=gcc V=1):
#   V		Verbose mode (example: make V=1)
#   CC		Override CC (default clang)
#   CI		Set CI=1 for CI pipeline
#   TARGET  Linux, mingw64

include versions.mk

CI := 0

MAKE += -j CC=$(CC)

ifeq ($(V),)
HIDE=@
MAKE += --no-print-directory
endif

TARGET := Linux


##############################################################################
#
# Target Configuration
#

ifeq ($(TARGET), mingw64)
	_ARCH := x86_64-w64-mingw32
	OPENSSL_TARGET := mingw64
	LIBVPX_TARGET := --target=x86_64-win64-gcc
	_ARCH_CONFIGURE := ./configure --host=$(_ARCH) --target=$(_ARCH) \
			   --libdir=/usr/$(_ARCH)/lib \
			   --includedir=/usr/$(_ARCH)/include
	export CMAKE_TOOLCHAIN_FILE := $(PWD)/cmake/mingw-w64-x86_64.cmake
	export MINGW_ARCH := 64
	export CC := $(_ARCH)-gcc
	export RANLIB :=${_ARCH}-ranlib
	export AR := ${_ARCH}-ar
	export CROSS := ${_ARCH}-
else
	_ARCH :=
	OPENSSL_TARGET :=
	LIBVPX_TARGET :=
	_ARCH_CONFIGURE := ./configure
	export CC := clang
	export CROSS :=
endif


##############################################################################
#
# Main
#

.PHONY: all
all: third_party external
	$(HIDE)echo $(OS)
	$(HIDE)[ -d build ] || cmake -B build -GNinja -DCMAKE_BUILD_TYPE=Debug
	$(HIDE)cmake --build build -j

##############################################################################
#
# Third Party section
#

.PHONY: openssl
openssl: third_party/openssl

.PHONY: opus
opus: third_party/opus

.PHONY: portaudio
portaudio: third_party/portaudio

.PHONY: samplerate
samplerate: third_party/libsamplerate

.PHONY: lmdb
lmdb: third_party/lmdb

.PHONY: libvpx
libvpx: third_party/libvpx

.PHONY: cacert
cacert: third_party/cacert.pem

.PHONY: third_party_dir
third_party_dir:
	mkdir -p third_party/include
	mkdir -p third_party/lib

.PHONY: third_party
third_party: third_party_dir libvpx openssl opus samplerate portaudio lmdb \
	cacert

third_party/libvpx:
	$(HIDE)cd third_party && \
	git clone --depth=1 --branch=${VPX_VERSION} ${VPX_MIRROR} && \
	mkdir build_vpx && cd build_vpx && \
	../libvpx/configure --prefix=../ $(LIBVPX_TARGET) \
	--enable-realtime-only \
	--disable-unit-tests --disable-vp9 --disable-tools --disable-shared \
	--enable-runtime-cpu-detect \
	--enable-pic \
	--disable-install-docs --disable-examples && \
	make -j4 && \
	make install

third_party/openssl:
	$(HIDE)cd third_party && \
		wget ${OPENSSL_MIRROR}/openssl-${OPENSSL_VERSION}.tar.gz && \
		tar -xzf openssl-${OPENSSL_VERSION}.tar.gz && \
		mv openssl-${OPENSSL_VERSION} openssl
	@rm -f third_party/openssl-${OPENSSL_VERSION}.tar.gz
	$(HIDE)cd third_party/openssl && \
		./config $(OPENSSL_TARGET) no-shared && \
		make -j32 build_libs && \
		cp *.a ../lib && \
		cp -a include/openssl ../include/

third_party/opus:
	$(HIDE)cd third_party && \
		wget ${OPUS_MIRROR}/opus-${OPUS_VERSION}.tar.gz && \
		tar -xzf opus-${OPUS_VERSION}.tar.gz && \
		mv opus-${OPUS_VERSION} opus
	$(HIDE)cd third_party/opus && \
		$(_ARCH_CONFIGURE) --with-pic --enable-dred \
		--enable-deep-plc --enable-osce --disable-extra-programs && \
		make -j && \
		cp .libs/libopus.a ../lib/ && \
		mkdir -p ../include/opus && \
		cp include/*.h ../include/opus/

third_party/portaudio:
	$(HIDE)cd third_party && \
		git clone ${PORTAUDIO_MIRROR}/portaudio.git && \
	    cd portaudio && \
		cmake -B build -DBUILD_SHARED_LIBS=0 \
		-DPA_USE_WMME=OFF -DPA_USE_DS=OFF -DPA_USE_WDMKS=OFF && \
		cmake --build build -j && \
		cp -a build/libportaudio.a ../lib/ && \
		cp include/*.h ../include/

third_party/libsamplerate:
	$(HIDE)cd third_party && \
		git clone ${SAMPLERATE_MIRROR}/libsamplerate.git && \
		cd libsamplerate && \
		cmake -B build -DBUILD_SHARED_LIBS=0 \
		-DBUILD_TESTING=OFF -DLIBSAMPLERATE_EXAMPLES=OFF && \
		cmake --build build -j && \
		cp -a build/src/libsamplerate.a ../lib/ && \
		cp include/*.h ../include/

third_party/lmdb:
	$(HIDE)cd third_party && \
		git clone https://github.com/LMDB/lmdb && \
		cd lmdb/libraries/liblmdb && \
		make CC=$(CC) -j && \
		cp liblmdb.a ../../../lib/ && \
		cp lmdb.h ../../../include/

third_party/cacert.pem:
	wget https://curl.se/ca/cacert.pem -O third_party/cacert.pem

.PHONY: external_dir
external_dir:
	mkdir -p external

external: external_dir external/re external/baresip

external/re:
	$(HIDE) [ ! -d external/re ] && \
		git -C external clone -b $(LIBRE_VERSION) --depth=1 \
		https://github.com/baresip/re.git

external/baresip:
	$(HIDE) [ ! -d external/baresip ] && \
		git -C external clone -b $(BARESIP_VERSION) --depth=1 \
		https://github.com/baresip/baresip.git


##############################################################################
#
# Tools & Cleanup
#

.PHONY: clean
clean:
	$(HIDE)rm -Rf build

.PHONY: cleaner
cleaner: clean
	$(HIDE)rm -Rf external

.PHONY: distclean
distclean: clean cleaner
	$(HIDE)rm -Rf third_party

.PHONY: ccheck
ccheck:
	test/ccheck.py libsl Makefile test app

.PHONY: tree
tree:
	tree -L 4 -I "third_party|node_modules|build*|external" -d .

.PHONY: test
test: all
	cppcheck libsl app test
	build/test/test
	#test/integration.sh

.PHONY: test_debug
test_debug: all
	gdb -batch -ex "run" -ex "bt" build/test/test

.PHONY: watch
watch:
	$(HIDE)while true; do \
	inotifywait -qr -e modify libsl test; \
	make test; sleep 0.5; \
	done

r: run
.PHONY: run
run: all
	build/app/cli/studiolink

.PHONY: dev
dev: all
	build/app/cli/studiolink --headless

.PHONY: release
release:
	make cleaner
	make external
	cmake -B build -GNinja -DCMAKE_BUILD_TYPE=Release
	make all

.PHONY: linux_debug
linux_debug: all
	readelf -d build/app/cli/studiolink | grep NEEDED

.PHONY: macos_debug
macos_debug: all
	otool -L build/app/cli/studiolink


##############################################################################
#
# Sanitizers
#

.PHONY: run_san
run_san:
	ASAN_OPTIONS=fast_unwind_on_malloc=0 \
	TSAN_OPTIONS="suppressions=tsan.supp" \
	make run

.PHONY: test_san
test_san:
	ASAN_OPTIONS=fast_unwind_on_malloc=0 \
	TSAN_OPTIONS="suppressions=tsan.supp" \
	make test

.PHONY: asan
asan: external
	make clean
	cmake -B build -GNinja -DCMAKE_BUILD_TYPE=Debug \
		-DCMAKE_C_FLAGS="-fsanitize=undefined,address \
		-fno-omit-frame-pointer" \
		-DHAVE_THREADS=
	make all

.PHONY: tsan
tsan: external
	make clean
	cmake -B build -GNinja -DCMAKE_BUILD_TYPE=Debug \
		-DCMAKE_C_FLAGS="-fsanitize=undefined,thread \
		-fno-omit-frame-pointer" \
		-DHAVE_THREADS=
	make all

.PHONY: msan
msan:
	make clean
	make external
	cd third_party/openssl && \
		make clean && \
		CC=$(CC) ./config no-shared enable-msan && \
		make -j build_libs && \
		cp *.a ../lib && \
		cp -a include/openssl ../include/
	cmake -B build -GNinja -DCMAKE_BUILD_TYPE=Debug \
	-DCMAKE_C_FLAGS="-fsanitize=undefined,memory -fno-omit-frame-pointer" \
	-DHAVE_THREADS=
	make all
