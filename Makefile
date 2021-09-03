#
# Makefile
#
# Copyright (C) 2021 Studio.Link Sebastian Reimers
#
VERSION   := 22.01.0

LIBRE_VERSION   := master
LIBREM_VERSION  := master
BARESIP_VERSION := master

MAKE += CC=clang -j4

.PHONY: libre
libre: third_party/re
	@rm -f third_party/re/libre.*
	$(MAKE) -C third_party/re libre.a

.PHONY: librem
librem: third_party/rem
	@rm -f third_party/rem/librem.*
	$(MAKE) -C third_party/rem librem.a

.PHONY: libbaresip
libbaresip: third_party/baresip
	@rm -f third_party/baresip/libbaresip.*
	$(MAKE) -C third_party/baresip libbaresip.a

third_party/re:
	$(shell [ ! -d third_party/re ] && \
		git -C third_party clone https://github.com/baresip/re.git)
	git -C third_party/re checkout $(LIBRE_VERSION)

third_party/rem:
	$(shell [ ! -d third_party/rem ] && \
		git -C third_party clone https://github.com/baresip/rem.git)
	git -C third_party/rem checkout $(LIBREM_VERSION)

third_party/baresip:
	$(shell [ ! -d third_party/baresip ] && \
		git -C third_party clone https://github.com/baresip/baresip.git)
	git -C third_party/baresip checkout $(BARESIP_VERSION)

.PHONY: third_party
third_party:
	mkdir -p third_party
	make libre
	make librem
	make libbaresip
