CC = gcc
CFLAGS = -Wall -Iinclude

src = $(wildcard src/*.c)
obj = $(src:.c=.o)

all: fg2019

fg2019: $(obj)
	$(CC) $(CFLAGS) $^ -o $@

obj:

.PHONY:
clean:
	rm src/*.o



