GCC = gcc
FLAGS = -Wall
FILES = files

all: servers/fifo client/client servers/fork servers/threads


servers/fifo: servers/fifo.c
	$(GCC) $(FLAGS) servers/fifo.c -o servers/fifo -lpthread

client/client: client/client.c
	$(GCC) $(FLAGS) client/client.c -o client/client -lpthread


servers/fork: servers/fork.c
	$(GCC) $(FLAGS) servers/fork.c -o servers/fork -lpthread


servers/threads: servers/threads.c
	$(GCC) $(FLAGS) servers/threads.c -o servers/threads -lpthread



# Test files generation
test-files:
	@echo "Creating test files..."
	@echo "<html><body><h1>Test Page</h1></body></html>" > $(FILES)/index.html
	@dd if=/dev/urandom of=$(FILES)/image.jpg bs=1K count=100 status=none
	@dd if=/dev/urandom of=$(FILES)/largefile.bin bs=1M count=10 status=none
	@echo "Test files created in $(FILES)/"







# Test a single server
test-server: servers/fifo servers/fork servers/threads
	./servers/fifo &
	./servers/fork &
	./servers/threads &

test-client:
	./client/client Hi.txt
	.




clean:
	rm -f servers/*.o client/*.o servers/fifo client/client servers/fork servers/threads