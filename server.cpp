#include <unistd.h> 
#include <iostream>
#include <cstdio>
#include <stdlib.h> 
#include <string>
#include <cstring>
#include <sys/socket.h> 
#include <netinet/in.h>  
#include <map>

using namespace std;

int generateClientId(map<int, string> *clientIds) {
	int randId = rand() % 899999 + 100000;
	if ((*clientIds).find(randId) != (*clientIds).end()) {
		randId = generateClientId(clientIds);
	}
	return randId;
}


int main(){

	map<int, string> clientIds;

	//Creating the socket
	int server_status, socket_status;
	struct sockaddr_in SocketAddress;
	int addrlen = sizeof(SocketAddress);
	server_status = socket(AF_INET, SOCK_STREAM, 0);
	if(server_status == 0){
		printf("Failed to create socket.\n");
		return -1;
	}
	//Links socket to port 8080
	SocketAddress.sin_family = AF_INET;
	SocketAddress.sin_addr.s_addr = INADDR_ANY;  
	SocketAddress.sin_port = htons(8080);
	if(bind(server_status, (struct sockaddr*)&SocketAddress, sizeof(SocketAddress)) == -1){
		printf("Failed to bind socket.\n");
		return -1;
	}
	//Wait for new client and try to accept it
	if(listen(server_status, SOMAXCONN) == -1){
		printf("Failed to listen.\n");
		return -1;
	}

  //while(1)
    socket_status = accept(server_status, (struct sockaddr*)&SocketAddress, (socklen_t*)&addrlen);
    if(socket_status == -1){
      printf("Failed to accept client.\n");
      return -1;
    }
    //Send welcome message
    string clientId = to_string(generateClientId(&clientIds));
    //char homepage[10] = clientId.c_str();
    send(socket_status, clientId.c_str(), strlen(clientId.c_str()), 0);

    //Start to read the client
    int read_status;
    char clientmsg[4096];
    while(true){
      memset(clientmsg, 0, sizeof(clientmsg));
      read_status = read(socket_status, clientmsg, sizeof(clientmsg));
      if(read_status <= 0) break;


      if (clientmsg[0] != '/')
        send(socket_status, clientmsg, strlen(clientmsg), 0);
      //else if
    }
	return 0;
}