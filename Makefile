
VERSION=0.dev

# normal CFLAGS
CFLAGS=-O2 -Wall

# CFLAGS without optimization
CFLAGSNO=-O0 -Wall

# targets
ALL: bin tmp bin/volrace bin/netplay bin/bufnet

bin:
	mkdir -p bin

tmp:
	mkdir -p tmp

src/version.h: Makefile
	echo "#define VERSION \""$(VERSION)"\""  > src/version.h

bin/volrace: src/version.h src/volrace.c
	$(CC) $(CFLAGS) -o bin/volrace src/volrace.c

tmp/net.o: src/net.h src/net.c
	$(CC) $(CFLAGS) -c -o tmp/net.o src/net.c

bin/netplay: src/version.h tmp/net.o src/netplay.c
	$(CC) $(CFLAGSNO) -o bin/netplay src/netplay.c tmp/net.o -lasound -lrt

bin/netplay_ALSANC: src/version.h tmp/net.o src/netplay.c
	$(CC) $(CFLAGSNO) -DALSANC -I$(ALSANC)/include -L$(ALSANC)/lib -o bin/netplay_ALSANC src/netplay.c tmp/net.o -lasound -lrt 

bin/netplay_static: src/version.h tmp/net.o src/netplay.c
	$(CC) $(CFLAGSNO) -DALSANC -I$(ALSANC)/include -L$(ALSANC)/lib -o bin/netplay_static src/netplay.c tmp/net.o -lasound -lrt -lpthread -lm -ldl -static

bin/bufnet: src/version.h tmp/net.o src/bufnet.c
	$(CC) $(CFLAGSNO) -D_FILE_OFFSET_BITS=64 -o bin/bufnet tmp/net.o src/bufnet.c -lrt

clean: 
	rm -rf src/version.h bin tmp

veryclean: clean
	rm -f *~ */*~ */*/*~



