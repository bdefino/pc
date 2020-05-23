CC := cc
CFLAGS := -Iinclude --std=c89 -Wall -Werror -Wpedantic
LDFLAGS := -lpthread

.PHONY: all clean

all: clean bin/queue-test bin/pc-test

bin:
	mkdir -p bin

bin/pc-test: bin build/pc.o build/pc-test.o build/queue.o
	$(CC) $(LDFLAGS) -o $@ build/pc*.o build/queue.o

bin/queue-test: bin build/queue.o build/queue-test.o
	$(CC) $(LDFLAGS) -o $@ build/queue*.o

build:
	mkdir -p build

build/pc.o: build include/queue.h include/pc.h src/pc.c
	$(CC) $(CFLAGS) -c -o $@ src/pc.c

build/pc-test.o: build include/pc.h src/pc-test.c
	$(CC) $(CFLAGS) -c -o $@ src/pc-test.c

build/queue.o: build include/queue.h src/queue.c
	$(CC) $(CFLAGS) -c -o $@ src/queue.c

build/queue-test.o: build include/queue.h src/queue-test.c
	$(CC) $(CFLAGS) -c -o $@ src/queue-test.c

clean:
	rm -rf bin build

