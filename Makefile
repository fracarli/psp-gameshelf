TARGET = gameshelf
OBJS = src/main.o

INCDIR =
CFLAGS = -O2 -G0 -Wall -Wextra -std=c99
CXXFLAGS = $(CFLAGS)
ASFLAGS = $(CFLAGS)

EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = PSP GameShelf 1.03
PSP_EBOOT_ICON = assets/ICON0.PNG
PSP_FW_VERSION = 661
LIBS = -lpspsystemctrl_user

PSPDEV ?= $(HOME)/pspdev
PSPSDK ?= $(PSPDEV)/psp/sdk
include $(PSPSDK)/lib/build.mak

src/main.o: src/xmb.h src/xmb_icon.h
