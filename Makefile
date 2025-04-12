GCC = gcc
FLAGS = -Wall


all: servers/fifo client/client
servers/fifo: servers/fifo.o
	$(GCC) $(FLAGS) servers/fifo.o -o servers/fifo

servers/fifo.o: servers/fifo.c
	$(GCC) $(FLAGS) -c servers/fifo.c -o servers/fifo.o

client/client: client/client.o
	$(GCC) $(FLAGS) client/client.o -o client/client -lpthread
client/client.o: client/client.c
	$(GCC) $(FLAGS) -c client/client.c -o client/client.o



run: client/client servers/fifo
	./servers/fifo
	./client/client

clean:
	rm -f servers/*.o client/*.o servers/fifo client/client