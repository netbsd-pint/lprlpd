CC=clang
CFlags= -g -Wall -pthread

DEPS = server.h threadpool.h
FILE = server.c threadpool.c

server: $(DEPS) $(FILE)
	$(CC) $(CFlags) -o server $(FILE)
	 
check: server $(DEPS) $(FILE)
	./server

clean:
	rm server