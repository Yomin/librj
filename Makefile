.PHONY: all, lib, clean, debug

all: librj.o

debug: librj

lib: librj.a

clean:
	find . -maxdepth 1 ! -type d \( -perm -111 -or -name "*\.a" -or -name "*\.o" -or -name "*\.test" \) -exec rm {} \;


librj.o: librj.c
	gcc -Wall -c -D NDEBUG -o $@ $<

librj.a: librj.o
	ar rcs $@ $<

librj: librj.c
	gcc -Wall -ggdb -o $@ $<
