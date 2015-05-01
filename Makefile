test: test.c
	$(CC) -std=c99 -pedantic -Wall -Wextra -o $@ $^
