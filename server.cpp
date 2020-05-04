#include <unistd.h> 
#include <iostream>
#include <cstdio>
#include <stdlib.h> 
#include <string>
#include <cstring>
#include <sys/socket.h> 
#include <netinet/in.h>  

using namespace std;

int main(){
  // Criando o socket
  int server_status, socket_status;
  struct sockaddr_in SocketAddress;
  int addrlen = sizeof(SocketAddress);
  server_status = socket(AF_INET, SOCK_STREAM, 0);
  if(server_status == 0){
    printf("Erro ao criar o socket.\n");
    return -1;
  }
  
  // Conecta o socket à porta 8080
  SocketAddress.sin_family = AF_INET;
  SocketAddress.sin_addr.s_addr = INADDR_ANY;  
  SocketAddress.sin_port = htons(8080);
  if(bind(server_status, (struct sockaddr*)&SocketAddress, sizeof(SocketAddress)) == -1){
    printf("Erro ao conectar o socket.\n");
    return -1;
  }
  
  // Aguarda a conexão com o cliente
  if(listen(server_status, SOMAXCONN) == -1){
    printf("Erro na conexão com o cliente.\n");
    return -1;
  }

  socket_status = accept(server_status, (struct sockaddr*)&SocketAddress, (socklen_t*)&addrlen);
  if(socket_status == -1){
    printf("Falha ao aceitar o cliente.\n");
    return -1;
  }

  // Manda a mensagem de boas vindas
  char homepage[] = "Conectado ao servidor!\n";
  send(socket_status, homepage, strlen(homepage), 0);

  //Start to read the client
  int read_status;
  char clientmsg[4096];
  while(true){
    memset(clientmsg, 0, sizeof(clientmsg));
    read_status = read(socket_status, clientmsg, sizeof(clientmsg));
    if(read_status <= 0) break;
    send(socket_status, clientmsg, strlen(clientmsg), 0);
    printf("Cliente diz: %s\n", clientmsg);
  }
  return 0;
}