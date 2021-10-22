#
# Makefile
#
# Copyright (C) 2021 Studio.Link Sebastian Reimers
#

LIBRE_PATH   := ../third_party/re
LIBREM_PATH  := ../third_party/rem
BARESIP_PATH := ../third_party/baresip
LIBRE_MK     := $(LIBRE_PATH)/mk/re.mk

include $(LIBRE_MK)

CFLAGS += -I../include -I$(LIBRE_PATH)/include -I$(LIBREM_PATH)/include
CFLAGS += -I$(BARESIP_PATH)/include

STATIC := libsl.a
APP_MK := srcs.mk

include $(APP_MK)

OBJS := $(patsubst %.c,$(BUILD)/%.o,$(filter %.c,$(SRCS)))

all: $(STATIC)

$(STATIC): $(OBJS)
	@echo "  AR	     $@"
	@$(AR) $(AFLAGS) $@ $(OBJS)
ifneq ($(RANLIB),)
	@$(RANLIB) $@
endif

$(BUILD)/%.o: %.c $(BUILD) Makefile $(APP_MK)
	@$(CC) $(CFLAGS) -c $< -o $@ $(DFLAGS)

$(BUILD): Makefile
	@mkdir -p $@
	@touch $@

.PHONY: clean
clean:
	@rm -rf $(STATIC) $(BUILD)
