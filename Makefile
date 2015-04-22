BIN_NAME = ev3duder	
# ^ exe should be removed

#CXX = g++
#C:/TDM/bin/g++
#CXX = C:/mingw/32/bin/g++	

CC = gcc
override FLAGS += -std=c11 -Wall -Wextra -fdiagnostics-color=auto

SRCDIR = src
OBJDIR = build

#LIBS = -lpsapi

SRCS = src/main.c src/cmd_defaults.c src/ev3_exec.c src/ev3_test.c src/ev3_up.c src/ev3_ls.c

# TODO: Apple's ld doesn't support interleaving -Bstatic
LIBS = -Llib/ -lhidapi 
INC += -Iinclude/
 
OBJS = $(SRCS:$(SRCDIR)/%.c=$(OBJDIR)/%.o) 


####################
# static enables valgrind to act better -DDEBUG!
debug: FLAGS += -g
debug: LIBS := $(LIBS)
debug: LIBS += 
debug: $(BIN_NAME)
.DEFAULT: (BIN_NAME)
$(BIN_NAME): $(OBJS)
	$(CC) $(OBJS) $(FLAGS) $(LIBS) -o $@

$(OBJS): $(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) -c $< -MMD $(FLAGS) $(INC) -o $@



-include $(OBJDIR)/*.d

ifeq ($(OS),Windows_NT)
RM = del /Q
endif

.PHONY: clean
clean:
	$(RM) $(BIN_NAME) && cd $(OBJDIR) && $(RM) *.o *.d 
