CC := cc
CFLAGS := -Iinclude --std=c89 -Wall -Werror -Wpedantic
LDFLAGS := -lpthread

.PHONY: all clean

all: clean bin/queue-test bin/consumer-test

bin:
	mkdir -p bin

bin/consumer-test: bin build/consumer.o build/consumer-test.o build/queue.o
	$(CC) $(LDFLAGS) -o $@ build/consumer*.o build/queue.o

bin/queue-test: bin build/queue.o build/queue-test.o
	$(CC) $(LDFLAGS) -o $@ build/queue*.o

build:
	mkdir -p build

build/consumer.o: build include/queue.h include/pc.h src/consumer.c
	$(CC) $(CFLAGS) -c -o $@ src/consumer.c

build/consumer-test.o: build include/pc.h src/consumer-test.c
	$(CC) $(CFLAGS) -c -o $@ src/consumer-test.c

build/queue.o: build include/queue.h src/queue.c
	$(CC) $(CFLAGS) -c -o $@ src/queue.c

build/queue-test.o: build include/queue.h src/queue-test.c
	$(CC) $(CFLAGS) -c -o $@ src/queue-test.c

clean:
	rm -rf bin build

