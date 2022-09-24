#
# Makefile
#
# Copyright (C) 2022 Studio.Link Sebastian Reimers
# Variables (make CC=gcc V=1):
#   V		Verbose mode (example: make V=1)
#   CC		Override CC (default clang)
#

include versions.mk

CC := clang

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
all: third_party
	[ -d build ] || cmake -B build -GNinja
	cmake --build build -j

.PHONY: info
info: third_party_dir third_party/re
	$(MAKE) -C libsl $@

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

.PHONY: third_party_dir
third_party_dir:
	mkdir -p third_party/include
	mkdir -p third_party/lib

.PHONY: third_party
third_party: third_party_dir openssl opus portaudio

third_party/openssl:
	$(HIDE)cd third_party && \
		wget ${OPENSSL_MIRROR}/openssl-${OPENSSL_VERSION}.tar.gz && \
		tar -xzf openssl-${OPENSSL_VERSION}.tar.gz && \
		mv openssl-${OPENSSL_VERSION} openssl
	@rm -f third_party/openssl-${OPENSSL_VERSION}.tar.gz
	$(HIDE)cd third_party/openssl && \
		./config no-shared && \
		make -j build_libs && \
		cp *.a ../lib && \
		cp -a include/openssl ../include/

third_party/opus:
	$(HIDE)cd third_party && \
		wget ${OPUS_MIRROR}/opus-${OPUS_VERSION}.tar.gz && \
		tar -xzf opus-${OPUS_VERSION}.tar.gz && \
		mv opus-${OPUS_VERSION} opus
	$(HIDE)cd third_party/opus && \
		./configure --with-pic && \
		make -j && \
		cp .libs/libopus.a ../lib/ && \
		mkdir -p ../include/opus && \
		cp include/*.h ../include/opus/

third_party/portaudio:
	$(HIDE)cd third_party && \
		wget ${PORTAUDIO_MIRROR}/v${PORTAUDIO_VERSION}.tar.gz && \
		tar -xzf v${PORTAUDIO_VERSION}.tar.gz && \
		mv portaudio-${PORTAUDIO_VERSION} portaudio
	$(HIDE)cd third_party/portaudio && \
		./configure && \
		make -j && \
		cp -a lib/.libs/libportaudio.a ../lib/ && \
		mkdir -p ../include/portaudio && \
		cp include/*.h ../include/portaudio/

third_party/re:
	mkdir -p third_party/include/re
	$(shell [ ! -d third_party/re ] && \
		git -C third_party clone https://github.com/baresip/re.git)
	git -C third_party/re checkout $(LIBRE_VERSION)

third_party/rem:
	mkdir -p third_party/include/rem
	$(shell [ ! -d third_party/rem ] && \
		git -C third_party clone https://github.com/baresip/rem.git)
	git -C third_party/rem checkout $(LIBREM_VERSION)

third_party/baresip:
	$(shell [ ! -d third_party/baresip ] && \
		git -C third_party clone \
		https://github.com/baresip/baresip.git)
	git -C third_party/baresip checkout $(BARESIP_VERSION)


##############################################################################
#
# Tools & Cleanup
#

.PHONY: clean
clean:
	$(HIDE)rm -Rf build

.PHONY: cleaner
cleaner: clean
	$(HIDE)rm -Rf third_party/re
	$(HIDE)rm -Rf third_party/rem
	$(HIDE)rm -Rf third_party/baresip

.PHONY: distclean
distclean: clean
	$(HIDE)rm -Rf third_party

.PHONY: ccheck
ccheck:
	test/ccheck.py libsl Makefile test app

.PHONY: tree
tree:
	tree -L 4 -I "third_party|node_modules|build*" -d .

.PHONY: test
test: libsl.a linux
	$(HIDE)$(MAKE) -C test
	$(HIDE)-$(MAKE) -C test compile_commands.json &
	$(HIDE)test/sltest
	$(HIDE)$(MAKE) -C test integration

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

