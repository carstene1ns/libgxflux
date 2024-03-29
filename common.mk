PREFIX ?= $(DEVKITPPC)/bin/powerpc-eabi-

AR = $(PREFIX)gcc-ar
AS = $(PREFIX)as
CC = $(PREFIX)gcc
CXX = $(PREFIX)g++
OBJCOPY = $(PREFIX)objcopy
RANLIB = $(PREFIX)gcc-ranlib
STRIP = $(PREFIX)strip

MACHDEP += -g -DGEKKO -mcpu=750 -meabi -mhard-float
CFLAGS += $(MACHDEP) -O2 -Wall -Wextra -pipe -I$(DEVKITPRO)/libogc/include
LDFLAGS += $(MACHDEP)
ASFLAGS += -D_LANGUAGE_ASSEMBLY -I$(DEVKITPRO)/libogc/include

ifeq ($(GAMECUBE),1)
MACHDEP += -mogc
CFLAGS += -DHW_DOL
LDFLAGS += -L$(DEVKITPRO)/libogc/lib/cube
ASFLAGS += -DHW_DOL
INSTALLINCLUDE = $(DEVKITPRO)/portlibs/gamecube/include
INSTALLLIB = $(DEVKITPRO)/portlibs/gamecube/lib
else
MACHDEP += -mrvl
CFLAGS += -DHW_RVL
LDFLAGS += -L$(DEVKITPRO)/libogc/lib/wii
ASFLAGS += -DHW_RVL
INSTALLINCLUDE = $(DEVKITPRO)/portlibs/wii/include
INSTALLLIB = $(DEVKITPRO)/portlibs/wii/lib
endif

ifneq ($(LDSCRIPT),)
LDFLAGS += -Wl,-T$(LDSCRIPT)
endif

DEPDIR = .deps

all: $(TARGET)

ifeq ($(LIBRARY),1)

$(TARGET): $(OBJS)
	@rm -f $@
	@echo "  AR        $@"
	@$(AR) cru $@ $+
	@echo "  RANLIB    $@"
	@$(RANLIB) $@

else

TARGET_STRIPPED = $(TARGET:.elf=_stripped.elf)

strip: $(TARGET_STRIPPED)

ifneq ($(LDSCRIPT),)
$(TARGET): $(LDSCRIPT)
endif

$(TARGET): $(OBJS)
	@echo "  LINK      $@"
	@$(CC) $(LDFLAGS) $+ $(LIBS) -o $@

$(TARGET_STRIPPED): $(TARGET)
	@echo "  STRIP     $@"
	@$(STRIP) $< -o $@

wiiload: $(TARGET_STRIPPED)
	@echo "  WIILOAD   $<"
	@$(DEVKITPRO)/tools/bin/wiiload $<

geckoload: $(TARGET_STRIPPED)
	@echo "  WIILOAD   $<"
	@WIILOAD=$(USBGECKODEVICE) $(DEVKITPRO)/tools/bin/wiiload $<

upload: geckoload
endif

%.o: %.c
	@echo "  COMPILE   $@"
	@mkdir -p $(DEPDIR)
	@$(CC) $(CFLAGS) $(DEFINES) -Wp,-MMD,$(DEPDIR)/$(*F).d,-MQ,"$@",-MP -c $< -o $@

%.o: %.s
	@echo "  ASSEMBLE  $@"
	@$(CC) $(CFLAGS) $(DEFINES) $(ASFLAGS) -x assembler-with-cpp -c $< -o $@

%.o: %.S
	@echo "  ASSEMBLE  $@"
	@$(CC) $(CFLAGS) $(DEFINES) $(ASFLAGS) -x assembler-with-cpp -c $< -o $@

clean:
	@rm -rf $(DEPDIR)
	@rm -f $(TARGET) $(TARGET_STRIPPED) $(TARGET).map $(OBJS)

-include $(DEPDIR)/*

.PHONY: clean
