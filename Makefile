
VERSION=0.6

# normal CFLAGS
CFLAGS=-O2 -Wall

# CFLAGS without optimization
CFLAGSNO=-O0 -Wall

# targets
ALL: bin tmp bin/volrace bin/bufhrt bin/highrestest \
     bin/writeloop bin/catloop bin/playhrt 

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

bin/playhrt: src/version.h tmp/net.o src/playhrt.c
	$(CC) $(CFLAGSNO) -o bin/playhrt src/playhrt.c tmp/net.o -lasound -lrt

bin/playhrt_ALSANC: src/version.h tmp/net.o src/playhrt.c
	$(CC) $(CFLAGSNO) -DALSANC -I$(ALSANC)/include -L$(ALSANC)/lib -o bin/playhrt_ALSANC src/playhrt.c tmp/net.o -lasound -lrt 

bin/playhrt_static: src/version.h tmp/net.o src/playhrt.c
	$(CC) $(CFLAGSNO) -DALSANC -I$(ALSANC)/include -L$(ALSANC)/lib -o bin/playhrt_static src/playhrt.c tmp/net.o -lasound -lrt -lpthread -lm -ldl -static

bin/bufhrt: src/version.h tmp/net.o src/bufhrt.c
	$(CC) $(CFLAGSNO) -D_FILE_OFFSET_BITS=64 -o bin/bufhrt tmp/net.o src/bufhrt.c -lrt

bin/highrestest: src/highrestest.c
	$(CC) $(CFLAGSNO) -o bin/highrestest src/highrestest.c -lrt

bin/writeloop: src/writeloop.c
	$(CC) $(CFLAGS) -o bin/writeloop src/writeloop.c

bin/catloop: src/catloop.c
	$(CC) $(CFLAGS) -o bin/catloop src/catloop.c

clean: 
	rm -rf src/version.h bin tmp

veryclean: clean
	rm -f *~ */*~ */*/*~

# private, for bin in distribution
bin86: 
	make veryclean
	make
	cc -O2 -Wall -o bin/volrace src/volrace.c -static
	cc -O0 -Wall -D_FILE_OFFSET_BITS=64 -o bin/bufhrt tmp/net.o src/bufhrt.c -static -lrt 
	cc -O0 -Wall -o bin/highrestest src/highrestest.c -static -lrt
	cc -O2 -Wall -o bin/writeloop src/writeloop.c -static
	cc -O2 -Wall -o bin/catloop src/catloop.c -static
	cc -O0 -Wall  -DALSANC -I$(ALSANC)/include -L$(ALSANC)/lib -o bin/playhrt src/playhrt.c tmp/net.o -lasound -lrt -lpthread -lm -ldl -static -lasound
	cd bin; \
	strip * ; \
	tar cvf frankl_stereo-$(VERSION)-bin-$(ARCH).tar * ; \
	gzip -9 frankl*.tar ; \
	mv frankl*gz ..
binPi: 
	make veryclean
	make
	cc -O2 -Wall -o bin/volrace src/volrace.c -static
	cc -O0 -Wall -D_FILE_OFFSET_BITS=64 -o bin/bufhrt tmp/net.o src/bufhrt.c -static  
	cc -O0 -Wall -o bin/highrestest src/highrestest.c -static 
	cc -O2 -Wall -o bin/writeloop src/writeloop.c -static
	cc -O2 -Wall -o bin/catloop src/catloop.c -static
	cc -O0 -Wall  -DALSANC -I$(ALSANC)/include -L$(ALSANC)/lib -o bin/playhrt src/playhrt.c tmp/net.o -lasound -lpthread -lm -ldl -static -lasound
	cd bin; \
	strip * ; \
	tar cvf frankl_stereo-$(VERSION)-bin-$(ARCH).tar * ; \
	gzip -9 frankl*.tar ; \
	mv frankl*gz ..

	

