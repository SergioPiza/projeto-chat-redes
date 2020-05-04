#include <unistd.h> 
#include <iostream> 
#include <sys/socket.h> 
#include <stdlib.h> 
#include <netinet/in.h> 
#include <string.h> 
#include <sstream>
#include <queue>

using namespace std;

// Função para parsear a mensagem em blocos de 4096 caracteres
queue<string> parseMessage() {
    queue<string> texts;
    string input;
    getline(cin, input);
    istringstream iss(input);
    string phrase;
    string buffer;
    while (getline(iss, phrase, ' ')) {
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

int main(){
	int socket_status;
	struct sockaddr_in ServerAddress;

	// Criando um socket
	socket_status = socket(AF_INET, SOCK_STREAM, 0);

	// Se a funcao anterior retorna -1 o novo servidor nao pode ser criado
	if(socket_status == -1){
		cout << "Erro ao criar o socket (cliente)\n";
		return 0;
	}


	ServerAddress.sin_family = AF_INET;
	ServerAddress.sin_port = htons(8080);

	//Conectando o cliente a porta 8080
	int retConnect = connect(socket_status, (struct sockaddr*) &ServerAddress, sizeof ServerAddress);
	if(retConnect == -1){
		cout << "Erro ao conectar à porta 8080 (cliente)\n";
		return 0;
	}

	char serverMessage[4097]; // variável que armazena a mensagem do servidor
	memset(serverMessage, 0, sizeof serverMessage); // inicializando a variável da mensagem
	read(socket_status, serverMessage, sizeof serverMessage); // recebendo a mensagem de boas vindas do servidor para testes
	cout << serverMessage << endl; // printando a mensagem
	
	bool quit = false;

    queue<string> message; // inicializa a variável que guardará a mensagem em filas de 4096 caracteres
    while(true) {
        message = parseMessage();
		
		if (message.size() == 1 && !message.front().compare("quit.")) {
            quit = true;
        }

		if(quit) break;

        // envia a mensagem para o servidor
        while(!message.empty()) {
			char msg_aux[4096]; 
			strcpy(msg_aux, message.front().c_str());
		    send(socket_status, msg_aux, strlen(msg_aux), 0);
            message.pop();
        }

		memset(serverMessage, 0, sizeof serverMessage); // zerando a variavel da resposta do servidor
		int server = read(socket_status, serverMessage, sizeof serverMessage);
		if (server <= 0){
			break;
		}

		cout << "Servidor: " << serverMessage << endl;
	}

	close(socket_status); //fecha o socket
	return 0;
}