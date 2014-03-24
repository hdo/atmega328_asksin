include ../../Makefile.common

#vpath %.c src

# Includes
CFLAGS += -Iinc 
CFLAGS += -I../../conf

# Sources
SRCS  = src/cc1100.c \
src/clock.c \
src/delay.c \
src/display.c \
src/fncollection.c \
src/led.c \
src/math_utils.c \
src/rf_asksin.c \
src/rf_moritz.c \
src/rf_receive.c \
src/rf_send.c \
src/ringbuffer.c \
src/serial.c \
src/spi.c \
src/stringfunc.c \
src/ttydata.c  

OBJS = $(SRCS:.c=.o)
LIBNAME = libasksin.a

all: $(LIBNAME)
	$(SIZE) $(LIBNAME)

%.o : %.c
	$(CC) $(CFLAGS) -c -o $@ $^

$(LIBNAME): $(OBJS)
	$(AR) -r $@ $(OBJS)

clean:
	rm -f $(OBJS) $(LIBNAME)
	