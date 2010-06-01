CFLAGS = -W -Wall -pedantic -std=c99 -Os

trie1: trie1.o

test: clean
	$(MAKE) "CFLAGS=$(CFLAGS) -DDEBUG"
	./trie1

clean:
	$(RM) trie1 *.o
