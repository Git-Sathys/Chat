
all: server.out client.out
server.out: server.o
	gcc -O3 -Wall -pthread -o server.out server.o
client.out: client.o string.o
	gcc -O3 -Wall -pthread -o client.out client.o string.o
server.o: src/server.c
	gcc -O3 -Wall -c src/server.c
string.o: src/string.c
	gcc -O3 -Wall -c src/string.c
client.o: src/client.c
	gcc -O3 -Wall -c src/client.c

.PHONY: clean
clean:
	rm -f *.o *.out