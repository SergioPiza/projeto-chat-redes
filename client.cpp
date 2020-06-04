#include <unistd.h> 
#include <iostream> 
#include <sys/socket.h> 
#include <stdlib.h> 
#include <netinet/in.h> 
#include <string.h> 
#include <string> 
#include <sstream>
#include <queue>
#include <cstdlib>
#include <csignal>

using namespace std;

// Função para parsear a mensagem em blocos de 4096 caracteres
queue<string> parseMessage() {
    queue<string> texts;
    string input;
    getline(cin, input);
    istringstream iss(input);
    string phrase;
    string buffer;
    while (getline(iss, phrase, '.')) {
        phrase.append(".");
        if (buffer.length() + phrase.length() > 4096) {
            texts.push(buffer);
            buffer = "";
        }
        buffer.append(phrase);
    }
    texts.push(buffer);

    return texts;
}

bool connect (int *socket_status, struct sockaddr_in *ServerAddress, string *clientId) {
	// Criando um socket
	*socket_status = socket(AF_INET, SOCK_STREAM, 0);

	// Se a funcao anterior retorna -1 o novo servidor nao pode ser criado
	if(*socket_status == -1){
		cout << "Creating Failed\n";
		return false;
	}

	ServerAddress->sin_family = AF_INET;
	ServerAddress->sin_port = htons(8080);

	//Conectando o cliente a porta 8080
	int retConnect = connect(*socket_status, (struct sockaddr*) ServerAddress, sizeof *ServerAddress);
	if(retConnect == -1){
		cout << "Connection Failed\n";
		return false;
	}

	char serverMessage[4097]; // variável que armazena a mensagem do servidor
	memset(serverMessage, 0, sizeof serverMessage); // inicializando a variável da mensagem
	read(*socket_status, serverMessage, sizeof serverMessage); // recebendo a mensagem de boas vindas do servidor para testes

	*clientId = serverMessage; // o servidor responde com o clientId do novo cliente
	
	return true;
}

void ctrlHandler(int signum){
	signal(SIGINT, ctrlHandler);
	cout << "Ignoring ctrl-c command." << endl;
}

int main(){

	signal(SIGINT, ctrlHandler);

	//Setup do socket
	int socket_status;
	struct sockaddr_in ServerAddress;

	bool quit = false;
	bool connected = false;

	string clientId;

    queue<string> message; // inicializa a variável que guardará a mensagem em filas de 4096 caracteres
 
 	char serverMessage[4097]; // variável que armazena a mensagem do servidor
    while(true) {
        message = parseMessage();
		
		if (message.size() == 1 && !message.front().compare("/quit.")) {
            quit = true;
        }

		if(quit) break;

        // envia a mensagem para o servidor
        while(!message.empty()) {
			char msg_aux[4096]; 
			strcpy(msg_aux,  message.front().c_str());

			if (msg_aux[0] == '/') {
				if (!strcmp(msg_aux, "/connect.")) {
					connected = connect(&socket_status, &ServerAddress, &clientId);
				}
			}

			if (connected) {
				string msg = clientId + ": " + msg_aux;
		    	send(socket_status, msg.c_str() , strlen(msg.c_str()), 0);
			} else {
				cout << "Você ainda não está conectado! Use o comando /connect" << endl;
				break;
			}

            message.pop();
        }

		memset(serverMessage, 0, sizeof serverMessage); // zerando a variavel da resposta do servidor
		
		if (connected) {
			int server = read(socket_status, serverMessage, sizeof serverMessage);
			if (server <= 0){
				break;
			}
		}

		cout << serverMessage << endl;
	}

	close(socket_status); //fecha o socket
	return 0;
}