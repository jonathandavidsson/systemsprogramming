CC = gcc
CFLAGS = -g -std=gnu11 -Werror -Wall -Wextra -Wpedantic -Wmissing-declarations -Wmissing-prototypes -Wold-style-definition
LDFLAGS = -lpthread
OBJ = mdu.o queue.o
TARGET = mdu

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) -o $(TARGET) $(OBJ) $(LDFLAGS)

mdu.o: mdu.c queue.h
	$(CC) $(CFLAGS) -c mdu.c

queue.o: queue.c queue.h
	$(CC) $(CFLAGS) -c queue.c

clean:
	rm -f $(OBJ) $(TARGET)
