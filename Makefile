#
# Makefile
#
# Copyright (C) 2022 Studio.Link Sebastian Reimers
# Variables (make CC=gcc V=1 CORES=2):
#   V		Verbose mode (example: make V=1)
#   CC		Override CC (default clang)
#

VER_MAJOR := 22
VER_MINOR := 1
VER_PATCH := 0
VER_PRE   := alpha

include versions.mk

BARESIP_MODULES := ice dtls_srtp turn opus g711

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

default: third_party libsl.a

.PHONY: libsl.a
libsl.a:
	$(HIDE)$(MAKE) -C libsl $@
	$(HIDE)-$(MAKE) -C libsl compile_commands.json &

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

.PHONY: libre
libre: third_party/re openssl
	@rm -f third_party/re/libre.*
	$(MAKE) -C third_party/re SYSROOT_ALT=../. libre.a
	-$(MAKE) -C third_party/re SYSROOT_ALT=../. compile_commands.json
	cp -a third_party/re/libre.a third_party/lib/
	$(HIDE) install -m 0644 \
		$(shell find third_party/re/include -name "*.h") \
		third_party/include/re

.PHONY: librem
librem: third_party/rem libre
	@rm -f third_party/rem/librem.*
	$(MAKE) -C third_party/rem SYSROOT_ALT=../. librem.a
	-$(MAKE) -C third_party/rem SYSROOT_ALT=../. compile_commands.json
	cp -a third_party/rem/librem.a third_party/lib/
	$(HIDE) install -m 0644 \
		$(shell find third_party/rem/include -name "*.h") \
		third_party/include/rem

.PHONY: libbaresip
libbaresip: third_party/baresip opus libre librem
	@rm -f third_party/baresip/libbaresip.* \
		third_party/baresip/src/static.c
	$(MAKE) -C third_party/baresip SYSROOT_ALT=../. STATIC=1 \
		MODULES="$(BARESIP_MODULES)" \
		libbaresip.a
	-$(MAKE) -C third_party/baresip SYSROOT_ALT=../. compile_commands.json
	cp -a third_party/baresip/libbaresip.a third_party/lib/
	cp -a third_party/baresip/include/baresip.h third_party/include/

.PHONY: third_party_dir
third_party_dir:
	mkdir -p third_party/include
	mkdir -p third_party/lib

.PHONY: third_party
third_party: third_party_dir openssl opus libre librem libbaresip

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
	cd third_party && wget ${OPUS_MIRROR}/opus-${OPUS_VERSION}.tar.gz && \
		tar -xzf opus-${OPUS_VERSION}.tar.gz && \
		mv opus-${OPUS_VERSION} opus
	$(HIDE)cd third_party/opus && \
		./configure --with-pic && \
		make -j && \
		cp .libs/libopus.a ../lib/ && \
		mkdir -p ../include/opus && \
		cp include/*.h ../include/opus/

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

.PHONY: bareinfo
bareinfo:
	$(MAKE) -C third_party/baresip SYSROOT_ALT=../. \
		STATIC=1 MODULES="$(BARESIP_MODULES)" \
		bareinfo

##############################################################################
#
# Tools & Cleanup
#

.PHONY: clean
clean:
	$(HIDE)[ -d third_party/re ] && $(MAKE) -C libsl clean || true
	$(HIDE)[ -d third_party/re ] && $(MAKE) -C test clean || true
	$(HIDE)[ -d third_party/re ] && $(MAKE) -C app/linux clean || true

.PHONY: cleaner
cleaner: clean
	$(HIDE)rm -Rf third_party/re
	$(HIDE)rm -Rf third_party/rem
	$(HIDE)rm -Rf third_party/baresip
	$(HIDE)rm -Rf third_party/include/re
	$(HIDE)rm -Rf third_party/include/rem
	$(HIDE)rm -Rf third_party/include/baresip.h

.PHONY: distclean
distclean: clean
	$(HIDE)rm -Rf third_party

.PHONY: ccheck
ccheck:
	test/ccheck.py libsl Makefile test

.PHONY: tree
tree:
	tree -L 4 -I "third_party|node_modules|build*" -d .

.PHONY: test
test: libsl.a linux
	$(HIDE)$(MAKE) -C test
	$(HIDE)-$(MAKE) -C test compile_commands.json &
	$(HIDE)test/sltest
	$(HIDE)$(MAKE) -C test integration

linux: libsl.a
	$(HIDE)$(MAKE) -C app/linux

.PHONY: watch
watch:
	$(HIDE)while true; do \
	inotifywait -qr -e modify libsl test; \
	make test; sleep 0.5; \
	done

.PHONY: run
run: linux
	app/linux/studiolink

.PHONY: dev
dev: linux
	app/linux/studiolink --headless

