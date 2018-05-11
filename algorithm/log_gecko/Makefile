CC = gcc
CFLAGS = -g -std=c99
SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)
TARGET = test
 
all : $(TARGET)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)
 
$(TARGET) :
	$(CC) $(CFLAGS) -c $(SRCS)
 
clean :
	rm -f $(TARGET) *.o
