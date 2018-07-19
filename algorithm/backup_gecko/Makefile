CC = gcc
CFLAGS = -g -std=c99 -DSLC
LDFLAGS = -lm
SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)
TARGET = test

test: all

all : $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

clean :
	$(RM) -f $(TARGET) *.o
