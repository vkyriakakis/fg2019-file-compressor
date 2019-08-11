CC = gcc
CFLAGS = -Wall -Iinclude -DNDEBUG

src = $(wildcard src/*.c)
obj = $(src:.c=.o)

all: fg2019

fg2019: $(obj)
	$(CC) $(CFLAGS) $^ -o $@


obj:

.PHONY:
clean:
	rm src/*.o

format_src:
	clang-format -style=file -i src/*.c

format_inc:
	clang-format -style=file -i include/fg2019/*.h 

