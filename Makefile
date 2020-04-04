# the compiler: gcc for C program, define as g++ for C++
CC = gcc

# compiler flags:
#  -g    adds debugging information to the executable file
#  -Wall turns on most, but not all, compiler warnings
CFLAGS  = -O2 -Wall
LFLAGS  = -pthread

FILE1 = AsyncQueue/AsyncQueue
FILE2 = AsyncQueue/test

# the build target executable:
TARGET = asyncqueue

all: $(TARGET)

$(TARGET): $(FILE1).o $(FILE2).o
	$(CC) $(LFLAGS) -o $(TARGET) $(FILE1).o $(FILE2).o

$(FILE1).o: $(FILE1).c
	$(CC) $(CFLAGS) -c $(FILE1).c -o $(FILE1).o  

$(FILE2).o: $(FILE2).c
	$(CC) $(CFLAGS) -c $(FILE2).c -o $(FILE2).o 

clean:
	rm $(TARGET)
	rm $(FILE1).o
	rm $(FILE2).o
