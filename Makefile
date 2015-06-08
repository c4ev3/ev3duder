BIN_NAME = ev3duder	
# ^ exe should be removed
# CC = gcc
# tip: clang -Weverything shows all GNU extensions
override FLAGS += -std=c99 -Wall -Wextra

SRCDIR = src
OBJDIR = build

#LIBS = -lpsapi

SRCS = src/main.c src/cmd_defaults.c src/run.c src/test.c src/up.c src/ls.c src/rm.c src/mkdir.c src/mkrbf.c src/dl.c

# TODO: Apple's ld doesn't support interleaving -Bstatic
LIBS = -Llib/ -lhidapi 
INC += -Iinclude/
 
print-%  : ; @echo $* = $($*)
####################
ifeq ($(OS),Windows_NT)
RM = del /Q
FLAGS += -municode
BIN_NAME := $(addsuffix .exe, $(BIN_NAME))
else
SRCS += src/btunix.c
endif
OBJS = $(SRCS:$(SRCDIR)/%.c=$(OBJDIR)/%.o) 

.DEFAULT: all
all: binary

binary: $(OBJS)
	$(CC) $(OBJS) $(FLAGS) $(LIBS) -o $(BIN_NAME)
# static enables valgrind to act better -DDEBUG!
$(OBJS): $(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) -c $< -MMD $(FLAGS) $(INC) -o $@
-include $(OBJDIR)/*.d

# CC=/path/to/cross/cc will need to be passed to make
# TODO: remove all %zu prints altogether
win32: FLAGS += -municode -Wno-unused-value -D__USE_MINGW_ANSI_STDIO=1
win32: all

debug: FLAGS += -g
debug: LIBS := $(LIBS)
debug: LIBS += 
debug: binary

.PHONY: clean
clean:
	$(RM) $(BIN_NAME) && cd $(OBJDIR) && $(RM) *.o *.d 

