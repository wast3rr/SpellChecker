CC = gcc
CFLAGS = -Wall -std=c99 -g

spchk: spchk.c
	$(CC) $(CFLAGS) $^ -o spchk

test: 
	./spchk /usr/share/dict/words .

clean:
	rm -f *.o spchk
