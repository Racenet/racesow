OBJECTS= skmodel.o

#use this line for profiling
#PROFILEOPTION=-pg -g
#use this line for no profiling
PROFILEOPTION=

#note:
#the -Werror can be removed to compile even if there are warnings,
#this is used to ensure that all released versions are free of warnings.

#normal compile
OPTIMIZATIONS= -O6 -ffast-math -fexpensive-optimizations
CFLAGS= -MD -Wall -Werror $(OPTIMIZATIONS) $(PROFILEOPTION)
#debug compile
#OPTIMIZATIONS= -O -g
#CFLAGS= -MD -Wall -Werror -ggdb $(OPTIMIZATIONS) $(PROFILEOPTION)

LDFLAGS= -lm $(PROFILEOPTION)

all: skmodel

.c.o:
	gcc $(CFLAGS) -c $*.c

skmodel: $(OBJECTS)
	gcc -o $@ $^ $(LDFLAGS)

clean:
        -rm -f skmodel *.o *.d

.PHONY: clean

-include *.d
