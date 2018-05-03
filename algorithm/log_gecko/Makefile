CC = gcc
INCLUDE = ./header
CFLAGS = -g -std=c99 -I$(INCLUDE)
SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)
TARGET = test
 
all : $(TARGET)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)
 
$(TARGET) :
	$(CC) $(CFLAGS) -c $(SRCS)
 
clean :
	rm -f $(TARGET) *.o
