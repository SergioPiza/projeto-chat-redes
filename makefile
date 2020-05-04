all:
	g++ server.cpp -ansi -o server
	g++ client.cpp -ansi -o client

runs:
	./server

runc:
	./client