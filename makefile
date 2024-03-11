CC = gcc
CFLAGS = -Wall -std=c99 -g -fsanitize=address,undefined

spchk: spchk.c
	$(CC) $(CFLAGS) $^ -o spchk

test: 
	./spchk /usr/share/dict/words .

clean:
	rm -f *.o spchk
