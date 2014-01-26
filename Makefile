
VERSION=0.dev

# normal CFLAGS
CFLAGS=-O2 -Wall

# CFLAGS without optimization
CFLAGSNO=-O0 -Wall

# targets
ALL: bin bin/volrace bin/netplay

bin:
	mkdir -p bin

src/version.h: Makefile
	echo "#define VERSION \""$(VERSION)"\""  > src/version.h

bin/volrace: src/version.h src/volrace.c
	$(CC) $(CFLAGS) -o bin/volrace src/volrace.c

bin/net.o: src/net.h src/net.c
	$(CC) $(CFLAGS) -c -o bin/net.o src/net.c

bin/netplay: src/version.h bin/net.o src/netplay.c
	$(CC) $(CFLAGSNO) -o bin/netplay src/netplay.c bin/net.o -lasound -lrt

bin/netplay_ALSANC: src/version.h bin/net.o src/netplay.c
	$(CC) $(CFLAGSNO) -DALSANC -I$(ALSANC)/include -L$(ALSANC)/lib -o bin/netplay_ALSANC src/netplay.c bin/net.o -lasound -lrt 

bin/netplay_static: src/version.h bin/net.o src/netplay.c
	$(CC) $(CFLAGSNO) -DALSANC -I$(ALSANC)/include -L$(ALSANC)/lib -o bin/netplay_static src/netplay.c bin/net.o -lasound -lrt -lpthread -lm -ldl -static

clean: 
	rm -rf src/version.h bin

veryclean: clean
	rm -f *~ */*~ */*/*~



