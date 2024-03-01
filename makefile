CC = gcc
CFLAGS = -Wall -std=c99 -g

spchk: spchk.c
	$(CC) $(CFLAGS) $^ -o spchk

clean:
	rm -f *.o spchk
