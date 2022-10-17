#
# Makefile
#
# Copyright (C) 2022 Studio.Link Sebastian Reimers
# Variables (make CC=gcc V=1):
#   V		Verbose mode (example: make V=1)
#   CC		Override CC (default clang)
#   CI		Set CI=1 for CI pipeline
#

include versions.mk

CC := clang

CI := 0

MAKE += -j CC=$(CC)

ifeq ($(V),)
HIDE=@
MAKE += --no-print-directory
endif

##############################################################################
#
# Main
#

.PHONY: all
all: third_party external
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

.PHONY: cacert
cacert: third_party/cacert.pem

.PHONY: third_party_dir
third_party_dir:
	mkdir -p third_party/include
	mkdir -p third_party/lib

.PHONY: third_party
third_party: third_party_dir openssl opus samplerate portaudio lmdb cacert

third_party/openssl:
	$(HIDE)cd third_party && \
		wget ${OPENSSL_MIRROR}/openssl-${OPENSSL_VERSION}.tar.gz && \
		tar -xzf openssl-${OPENSSL_VERSION}.tar.gz && \
		mv openssl-${OPENSSL_VERSION} openssl
	@rm -f third_party/openssl-${OPENSSL_VERSION}.tar.gz
	$(HIDE)cd third_party/openssl && \
		CC=$(CC) ./config no-shared && \
		make -j build_libs && \
		cp *.a ../lib && \
		cp -a include/openssl ../include/

third_party/opus:
	$(HIDE)cd third_party && \
		wget ${OPUS_MIRROR}/opus-${OPUS_VERSION}.tar.gz && \
		tar -xzf opus-${OPUS_VERSION}.tar.gz && \
		mv opus-${OPUS_VERSION} opus
	$(HIDE)cd third_party/opus && \
		CC=$(CC) ./configure --with-pic && \
		make -j && \
		cp .libs/libopus.a ../lib/ && \
		mkdir -p ../include/opus && \
		cp include/*.h ../include/opus/

third_party/portaudio:
	$(HIDE)cd third_party && \
		git clone ${PORTAUDIO_MIRROR}/portaudio.git && \
	    cd portaudio && \
		CC=$(CC) cmake -B build -DBUILD_SHARED_LIBS=0 && \
		cmake --build build -j && \
		cp -a build/libportaudio.a ../lib/ && \
		cp include/*.h ../include/

third_party/libsamplerate:
	$(HIDE)cd third_party && \
		git clone ${SAMPLERATE_MIRROR}/libsamplerate.git && \
		cd libsamplerate && \
		./autogen.sh && \
		CC=$(CC) ./configure --enable-static && \
		make -j && \
		cp src/.libs/libsamplerate.a ../lib/ && \
		cp include/samplerate.h ../include/

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

external: external_dir external/re external/rem external/baresip

external/re:
	$(shell [ ! -d external/re ] && \
		git -C external clone https://github.com/baresip/re.git)
	git -C external/re checkout $(LIBRE_VERSION)

external/rem:
	$(shell [ ! -d external/rem ] && \
		git -C external clone https://github.com/baresip/rem.git)
	git -C external/rem checkout $(LIBREM_VERSION)

external/baresip:
	$(shell [ ! -d external/baresip ] && \
		git -C external clone \
		https://github.com/baresip/baresip.git)
	git -C external/baresip checkout $(BARESIP_VERSION)


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
	test/integration.sh

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
	build/app/linux/studiolink

.PHONY: dev
dev: all
	build/app/linux/studiolink --headless

.PHONY: release
release:
	make cleaner
	make external
	cmake -B build -GNinja -DCMAKE_BUILD_TYPE=Release
	make all

.PHONY: linux_debug
linux_debug: all
	readelf -d build/app/linux/studiolink | grep NEEDED

.PHONY: macos_debug
macos_debug: all
	otool -L build/app/macos/studiolink


##############################################################################
#
# Sanitizers
#

.PHONY: run_san
run_san:
	TSAN_OPTIONS="suppressions=tsan.supp" make run

.PHONY: asan
asan:
	make clean
	make external
	cmake -B build -GNinja -DCMAKE_BUILD_TYPE=Debug \
		-DCMAKE_C_FLAGS="-fsanitize=undefined,address \
		-fno-omit-frame-pointer" \
		-DHAVE_THREADS=
	make all

.PHONY: tsan
tsan:
	make clean
	make external
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
