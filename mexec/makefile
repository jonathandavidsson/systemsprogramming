CC = gcc
CFLAGS = -g -std=gnu11 -Werror -Wall -Wextra -Wpedantic -Wmissing-declarations -Wmissing-prototypes -Wold-style-definition
VALGRINDFLAGS = valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --verbose
LEAKSFLAGS = leaks --atExit --

OBJS = mexec.o readcommands.o

mexec: $(OBJS)
	$(CC) $(CFLAGS) -o mexec $(OBJS)

mexec.o: mexec.c readcommands.h	
	$(CC) $(CFLAGS) -c mexec.c

readcommands.o: readcommands.c readcommands.h
	$(CC) $(CFLAGS) -c readcommands.c

runvalgrind: mexec
	$(VALGRINDFLAGS) ./mexec

runleaks: mexec
	$(LEAKSFLAGS) ./mexec

clean: 
	rm -f mexec *.o