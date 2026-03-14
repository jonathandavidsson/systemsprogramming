CC = gcc
CFLAGS = -g -std=gnu11 -Werror -Wall -Wextra -Wpedantic \
         -Wmissing-declarations -Wmissing-prototypes -Wold-style-definition
OBJ = mmake.o parser.o

mmake: $(OBJ)
	$(CC) $(CFLAGS) -o mmake $(OBJ)

mmake.o: mmake.c parser.h
	$(CC) $(CFLAGS) -c mmake.c

parser.o: parser.c parser.h
	$(CC) $(CFLAGS) -c parser.c

clean:
	rm -f $(OBJ) $(TARGET)