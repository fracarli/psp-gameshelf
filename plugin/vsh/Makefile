TARGET = gameshelf_vsh
OBJS = src/main.o

INCDIR =
CFLAGS = -O2 -G0 -Wall -Wextra -std=c99
CXXFLAGS = $(CFLAGS)
ASFLAGS = $(CFLAGS)

BUILD_PRX = 1
PSP_FW_VERSION = 661
USE_KERNEL_LIBS = 1
LIBS = -lpspsystemctrl_kernel

PSPDEV ?= /Users/francescocarli/pspdev
PSPSDK ?= $(PSPDEV)/psp/sdk
include $(PSPSDK)/lib/build_prx.mak
