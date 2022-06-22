#
# Makefile
#
# Copyright (C) 2021-2022 Studio.Link Sebastian Reimers
#

PROJECT      := sl

SYSROOT_ALT  := ../third_party
LIBRE_PATH   := ../third_party/re
LIBREM_PATH  := ../third_party/rem
BARESIP_PATH := ../third_party/baresip
LIBRE_MK     := $(LIBRE_PATH)/mk/re.mk

include $(LIBRE_MK)

CFLAGS += -Iinclude -I$(LIBRE_PATH)/include -I$(LIBREM_PATH)/include
CFLAGS += -I$(BARESIP_PATH)/include

STATIC := libsl.a
APP_MK := src/srcs.mk

include $(APP_MK)

OBJS := $(patsubst %.c,$(BUILD)/src/%.o,$(filter %.c,$(SRCS)))

all: $(STATIC)

$(STATIC): $(OBJS)
	@echo "  AR	     $@"
	@$(AR) $(AFLAGS) $@ $(OBJS)
ifneq ($(RANLIB),)
	@$(RANLIB) $@
endif

$(BUILD)/%.o: %.c $(BUILD) Makefile $(APP_MK)
	@echo "  CC	     $@"
	@$(CC) $(CFLAGS) -c $< -o $@ $(DFLAGS)

$(BUILD): Makefile assets
	@mkdir -p $@/src
	@touch $@

.PHONY: clean
clean:
	@rm -rf $(STATIC) $(BUILD) assets

.PHONY: assets
assets:
	@rm -Rf assets
	@mkdir -p assets
	@cd ../webui/ && xxd -i dist/index.html > ../libsl/assets/index_html.h
	@cd ../webui/ && xxd -i dist/index.js > ../libsl/assets/index_js.h
	@cd ../webui/ && xxd -i dist/vendor.js > ../libsl/assets/vendor_js.h
	@cd ../webui/ && xxd -i dist/index.css > ../libsl/assets/index_css.h
	@cd ../webui/ && xxd -i dist/roboto-mono-latin-400.woff2 \
		> ../libsl/assets/roboto-mono-latin-400_woff2.h
	@cd ../webui/ && xxd -i dist/roboto-mono-latin-500.woff2 \
		> ../libsl/assets/roboto-mono-latin-500_woff2.h
	@cd ../webui/ && xxd -i dist/roboto-mono-latin-600.woff2 \
		> ../libsl/assets/roboto-mono-latin-600_woff2.h
	@cd ../webui/ && xxd -i dist/roboto-mono-latin-700.woff2 \
		> ../libsl/assets/roboto-mono-latin-700_woff2.h
	@cd ../webui/ && xxd -i dist/logo_standalone.svg \
		> ../libsl/assets/logo_standalone_svg.h
