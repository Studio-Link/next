#
# Makefile
#
# Copyright (C) 2021 Studio.Link Sebastian Reimers
# Imported variables:
#   V			Verbose mode (example: make V=1)
#   CPU_CORES	Override CPU Core detection
#

VER_MAJOR := 22
VER_MINOR := 1
VER_PATCH := 0
VER_PRE   := alpha

#OPENSSL_VERSION :=
#OPUS_VERSION	:=
LIBRE_VERSION   := master
LIBREM_VERSION  := master
BARESIP_VERSION := master

BARESIP_MODULES := account opus vp8 portaudio

ifeq ($(OS),darwin)
CPU_CORES := $(shell sysctl -n hw.ncpu)
else
CPU_CORES := $(shell nproc)
endif

MAKE += -j$(CPU_CORES)

ifeq ($(V),)
HIDE=@
MAKE += --no-print-directory
endif

default: third_party studiolink.a

.PHONY: studiolink.a
studiolink.a: libre librem libbaresip
	$(MAKE) -C src $@

.PHONY: libre
libre: third_party/re
	@rm -f third_party/re/libre.*
	$(MAKE) -C third_party/re libre.a
	cp -a third_party/re/libre.a third_party/lib/
	$(HIDE) install -m 0644 \
		$(shell find third_party/re/include -name "*.h") \
		third_party/include/re

.PHONY: librem
librem: third_party/rem libre
	@rm -f third_party/rem/librem.*
	$(MAKE) -C third_party/rem librem.a
	cp -a third_party/rem/librem.a third_party/lib/
	$(HIDE) install -m 0644 \
		$(shell find third_party/rem/include -name "*.h") \
		third_party/include/rem

.PHONY: libbaresip
libbaresip: third_party/baresip libre librem
	@rm -f third_party/baresip/libbaresip.* \
		third_party/baresip/src/static.c
	$(MAKE) -C third_party/baresip STATIC=1 \
		MODULES="$(BARESIP_MODULES)" \
		libbaresip.a
	cp -a third_party/baresip/libbaresip.a third_party/lib/
	cp -a third_party/baresip/include/baresip.h third_party/include/

.PHONY: third_party_dir
third_party_dir:
	mkdir -p third_party/include
	mkdir -p third_party/lib

.PHONY: third_party
third_party: third_party_dir libre librem libbaresip

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
	$(MAKE) -C third_party/baresip STATIC=1 MODULES="$(BARESIP_MODULES)" \
		bareinfo

.PHONY: clean
clean:
	$(HIDE)$(MAKE) -C third_party/baresip clean
	$(HIDE)$(MAKE) -C third_party/rem clean
	$(HIDE)$(MAKE) -C third_party/re clean
	$(HIDE)$(MAKE) -C src clean

.PHONY: distclean
distclean:
	rm -Rf third_party

.PHONY: ccheck
ccheck:
	tests/ccheck.py src Makefile

.PHONY: tree
tree:
	tree -L 4 -I "third_party|node_modules" -d .
