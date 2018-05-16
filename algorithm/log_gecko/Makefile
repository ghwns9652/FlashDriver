CC = gcc
CFLAGS = -g -std=c99
LDFLAGS = -lm
SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)
TARGET = test
 
all : $(TARGET)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)
 
$(TARGET) :
	$(CC) $(CFLAGS) -c $(SRCS) $(LDFLAGS)
 
clean :
	rm -f $(TARGET) *.o
