CC = gcc
CFLAGS = -g -std=c99
LDFLAGS = -lm
SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)
TARGET = test

test: all

all : $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)
 
#$(TARGET) :
#	$(CC) $(CFLAGS) -c $(SRCS) $(LDFLAGS)

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

clean :
	$(RM) -f $(TARGET) *.o
