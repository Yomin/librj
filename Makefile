.PHONY: all, debug, lib, debuglib, clean, touch

all: debug =
all: touch librj.a

debug: debug = -ggdb
debug: touch librj.a

clean:
	find . -maxdepth 1 ! -type d \( -perm -111 -or -name "*\.a" -or -name "*\.o" -or -name "*\.test" \) -exec rm {} \;


rj.o: rj.c
	gcc -Wall -c $(debug) -D NDEBUG -o $@ $<

librj.a: rj.o
	ar rcs $@ $<

test: rj.c
	gcc -Wall -ggdb -o $@ $<


touch:
	$(shell [ -f debug -a -z "$(debug)" ] && { touch rj.c; rm debug; })
	$(shell [ ! -f debug -a -n "$(debug)" ] && { touch rj.c; touch debug; })
