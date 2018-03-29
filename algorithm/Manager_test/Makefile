CC = gcc
TARGET = test

OBJECTS = BM_Block.o\
		  BM_Heap.o\
		  BM_Interface.o\
		  PM_blockqueue.o\
		  PM_operation.o\
		  Manager_tester.o\

$(TARGET): $(OBJECTS)
		$(CC) $(CFLAGS) -o $@ $^

clean:
		rm *.o test


