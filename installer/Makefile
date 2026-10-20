TARGET = gameshelf_setup
OBJS = main.o
INCDIR = ../build/installer
CFLAGS = -O2 -G0 -Wall -Wextra -std=c99
EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = GameShelf Setup 1.09
PSP_EBOOT_ICON = ../assets/ICON0.PNG
PSP_FW_VERSION = 661
PSPDEV ?= $(HOME)/pspdev
PSPSDK ?= $(PSPDEV)/psp/sdk
include $(PSPSDK)/lib/build.mak
main.o: install.h ../build/installer/payload.h
