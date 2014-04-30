.PHONY: all, debug, clean, test, touch

CFLAGS := $(CFLAGS) -Wall -pedantic -std=c99
SOURCES = $(shell find . -maxdepth 1 -name "*.c")
OBJECTS = $(SOURCES:%.c=%.o)
NAME = rj

all: debug = no
all: CFLAGS := $(CFLAGS) -D NDEBUG
all: touch lib$(NAME).a

debug: debug = yes
debug: CFLAGS := $(CFLAGS) -ggdb
debug: touch lib$(NAME).a

clean:
	find . -maxdepth 1 ! -type d \( -perm -111 -or -name "*\.a" -or -name "*\.o" -or -name "*\.test" \) -exec rm {} \;


%.o: %.c
	gcc -c $(CFLAGS) -o $@ $<

lib$(NAME).a: $(OBJECTS)
	ar rcs $@ $(OBJECTS)

test: $(NAME).c
	gcc $(CFLAGS) -ggdb -D TEST -o $@ $<


touch:
	$(shell [ -f debug -a "$(debug)" = "no" ] && { touch *.c; rm debug; })
	$(shell [ ! -f debug -a "$(debug)" = "yes" ] && { touch *.c; touch debug; })
