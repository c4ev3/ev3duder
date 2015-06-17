##################################################
# Cross-Platform Makefile for the ev3duder utility
#
# Ahmad Fatoum
# Hochschule Aschaffenburg
# 2015-06-17
##################################################

BIN_NAME = ev3duder	
# tip: CC=clang FLAGS=-Weverything shows all GNU extensions
override FLAGS += -std=c99 -Wall -Wextra
SRCDIR = src
OBJDIR = build

SRCS = src/main.c src/cmd_defaults.c src/run.c src/test.c src/up.c src/ls.c src/rm.c src/mkdir.c src/mkrbf.c src/dl.c

# TODO: Apple's ld doesn't support interleaving -Bstatic
INC += -Ihidapi/hidapi/
 
print-%  : ; @echo $* = $($*)
####################
CREATE_BUILD_DIR := $(shell mkdir build)
ifeq ($(OS),Windows_NT)

## MinGW
ifneq ($(shell uname -o),Cygwin) 
RM = del /Q
endif

## Win32
# TODO: remove all %zu prints altogether
FLAGS += -Wno-unused-value -D__USE_MINGW_ANSI_STDIO=1
SRCS += src/btwin.c
HIDSRC += hidapi/windows/hid.c
LDFLAGS +=  -mwindows -lsetupapi -municode 
BIN_NAME := $(addsuffix .exe, $(BIN_NAME))
else

UNAME = $(shell uname -s)
## Linux
ifeq ($(UNAME),Linux)
HIDSRC += hidapi/libusb/hid.c
HIDFLAGS += `pkg-config libusb-1.0 --cflags`
LDFLAGS += `pkg-config libusb-1.0 --libs` -lrt -lpthread
endif

## OS X
ifeq ($(UNAME),Darwin)
HIDSRC += hidapi/mac/hid.c
endif

## BSD
ifeq ($(findstring BSD, $(UNAME)), BSD)
HIDSRC += hidapi/libusb/hid.c
LDFLAGS += -L/usr/local/lib -lusb -liconv -pthread
INC += -I/usr/local/include
endif

## ALL UNICES
SRCS += src/btunix.c
endif
OBJS = $(SRCS:$(SRCDIR)/%.c=$(OBJDIR)/%.o)

.DEFAULT: all
all: binary

binary: $(OBJS) $(OBJDIR)/hid.o
	$(CC) $(OBJS) $(OBJDIR)/hid.o $(LDFLAGS) $(LIBS) -o $(BIN_NAME)

# static enables valgrind to act better -DDEBUG!
$(OBJS): $(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) -c $< -MMD $(FLAGS) $(INC) -o $@
-include $(OBJDIR)/*.d

$(OBJDIR)/hid.o: $(HIDSRC)
	$(CC) -c $< -o $@ $(INC) $(HIDFLAGS)


# CC=/path/to/cross/cc will need to be passed to make

debug: FLAGS += -g
debug: LIBS := $(LIBS)
debug: LIBS += 
debug: binary

.PHONY: clean
clean:
	$(RM) $(BIN_NAME) && cd $(OBJDIR) && $(RM) *.o *.d 

