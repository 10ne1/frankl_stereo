
VERSION=0.dev

# normal CFLAGS
CFLAGS=-O2 -Wall

# CFLAGS without optimization
CFLAGSNO=-O0 -Wall

# targets
ALL: bin/volrace

src/version.h: Makefile
	echo "#define VERSION \""$(VERSION)"\""  > src/version.h

bin:
	mkdir -p bin

bin/volrace: bin src/version.h src/volrace.c
	$(CC) $(CFLAGS) -o bin/volrace src/volrace.c

clean: 
	rm -rf src/version.h bin

cleanwork: clean
	rm -f *~ */*~ */*/*~



