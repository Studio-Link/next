#
# Makefile
#
# Copyright (C) 2021 Studio.Link Sebastian Reimers
#

PROJECT      := sltest

SYSROOT_ALT  := ../third_party
LIBRE_PATH   := ../third_party/re
LIBREM_PATH  := ../third_party/rem
BARESIP_PATH := ../third_party/baresip
LIBRE_MK     := ../third_party/re/mk/re.mk

include $(LIBRE_MK)

CFLAGS += -Isrc -I../include -I$(LIBRE_PATH)/include -I$(LIBREM_PATH)/include
CFLAGS += -I$(BARESIP_PATH)/include
LFLAGS += -L../src # libsl.a

APP_MK := src/srcs.mk

BIN    := $(PROJECT)$(BIN_SUFFIX)
LIBS   := -lsl -lbaresip -lrem -lre -lm -lopus $(LIBS)

include $(APP_MK)

OBJS := $(patsubst %.c,$(BUILD)/src/%.o,$(filter %.c,$(SRCS)))

all: $(BIN)

$(BIN): $(OBJS)
	@echo "  LD      $@"
	@$(CC) $(LFLAGS) $(APP_LFLAGS) $^ $(LIBS) -o $@

$(BUILD)/%.o: %.c $(BUILD) Makefile $(APP_MK)
	@$(CC) $(CFLAGS) -c $< -o $@ $(DFLAGS)

$(BUILD): Makefile
	@mkdir -p $@/src
	@touch $@

.PHONY: clean
clean:
	@rm -rf $(BIN) $(BUILD)
