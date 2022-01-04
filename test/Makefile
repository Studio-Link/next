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

CFLAGS += -Isrc -I../libsl/include -I$(LIBRE_PATH)/include
CFLAGS += -I$(LIBREM_PATH)/include -I$(BARESIP_PATH)/include
LFLAGS += -L../libsl # libsl.a

APP_MK := src/srcs.mk

BIN    := $(PROJECT)$(BIN_SUFFIX)
LIBS   := -lsl -lbaresip -lrem -lre -lm -lopus $(LIBS)

include $(APP_MK)

OBJS := $(patsubst %.c,$(BUILD)/src/%.o,$(filter %.c,$(SRCS)))

-include $(OBJS:.o=.d)

all: $(BIN)

.PHONY: $(BIN)
$(BIN): $(OBJS)
	@echo "  LD      $@"
	@$(CC) $(LFLAGS) $(APP_LFLAGS) $^ $(LIBS) -o $@

$(BUILD)/%.o: %.c $(BUILD) Makefile $(APP_MK)
	@echo "  CC      $@"
	@$(CC) $(CFLAGS) -c $< -o $@ $(DFLAGS)

$(BUILD): Makefile
	@mkdir -p $@/src
	@touch $@

.PHONY: clean
clean:
	@rm -rf $(BIN) $(BUILD)

.PHONY: integration
integration:
	./integration.sh
