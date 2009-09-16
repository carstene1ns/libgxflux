LIBRARY = 1
TARGET = libgxflux.a

OBJS = gfx.o gfx_tex.o gfx_color.o gfx_con.o gfx_con_font.o
HEADERS = gfx.h gfx_types.h gfx_tex.h gfx_color.h gfx_con.h gfx_con_esc.h

include ../common.mk

ifeq ($(GAMECUBE),1)
INSTALLBASE = $(DEVKITPRO)/cube
else
INSTALLBASE = $(DEVKITPRO)/wii
endif

INSTALLLIB = $(INSTALLBASE)/lib
INSTALLINCLUDE = $(INSTALLBASE)/include/gfx

install: all
	@echo "  INSTALL   $(INSTALLBASE)"
	@mkdir -p $(INSTALLLIB)
	@mkdir -p $(INSTALLINCLUDE)
	@install -m 644 $(TARGET) $(INSTALLLIB)
	@install -m 644 $(HEADERS) $(INSTALLINCLUDE)

install-all:
	@$(MAKE) --no-print-directory clean
	@$(MAKE) --no-print-directory install
	@$(MAKE) --no-print-directory clean
	@$(MAKE) --no-print-directory GAMECUBE=1 install
	@$(MAKE) --no-print-directory GAMECUBE=1 clean


.PHONY: install

