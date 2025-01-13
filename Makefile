CC = gcc

client.o: client.c
	$(CC) -c client.c

server.o: server.c
	$(CC) -c server.c

init.o: init.c
	$(CC) -c init.c

client: client.o
	$(CC) -o client client.o

server: server.o
	$(CC) -o server server.o -pthread

init: init.o
	$(CC) -o init init.o

clean:
	rm -f client server init client.o server.o init.o

clear_data:
	rm -f *.dat

run_client: client
	./client

run_server: server
	./server

run_init: init
	rm -f *.dat
	./init