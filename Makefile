CC = gcc
CFLAGS = -W -Wall
TARGET = hw1
OBJECT = hw1.o
	
$(TARGET) : $(OBJECT)
	$(CC) $(CFLAGS) -o $@ $^

clean :
	rm *.o hw1
