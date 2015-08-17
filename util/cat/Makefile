CC = arm-none-linux-gnueabi-gcc
SRCS = cat.c ev3_lcd.c ev3_timer.c
OBJS = $(SRCS:%.c=%.o)

.DEFAULT: all
all: $(OBJS)
	$(CC) -o cat $(OBJS)

%.o: %.c
	$(CC) -c $<

