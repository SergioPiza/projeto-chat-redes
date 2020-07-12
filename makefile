all:
	g++ server.cpp -ansi -o server -std=c++11
	g++ client.cpp -ansi -pthread -o client -std=c++11

runs:
	./server

runc:
	./client