#include <stdio.h>
#include <iostream>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <string>
#include <thread>
#include <csignal>

#define PORT 5000  
#define BUFFER_SIZE 4096

using namespace std;

bool send_msg(int sock, string userInput){
    int sendRes = send(sock, userInput.c_str(), userInput.size(), 0);
    if(sendRes == -1)
        return false;
    return true;
}

int receive_msg(int sock, char buffer[BUFFER_SIZE]){
    memset(buffer, 0, BUFFER_SIZE);
    int bytesRecv = recv(sock, buffer, BUFFER_SIZE, 0);
    return bytesRecv;
}

void routine(int sock){
    while(1){
        char buffer[BUFFER_SIZE];
        int n = receive_msg(sock, buffer);
        if(n > 0){
            buffer[n] = '\0';
            printf("%s\n", buffer);
        }else if(n == -1){
            exit(EXIT_FAILURE);
        }
    }
}

// Ignores ctrl-c command
void ctrlHandler(int signum){
	signal(SIGINT, ctrlHandler);
}

int main(int argc, char const *argv[]){

    signal(SIGINT, ctrlHandler); // Ignores ctrl-c command

    char buffer[BUFFER_SIZE];
    int sock = 0; 
    string userInput;
    char userName[50] = {0};
    
    string ipAdress = "127.0.0.1"; // IP adress to connect
    sockaddr_in hint;
    hint.sin_family = AF_INET;
    hint.sin_port = htons(PORT);
    inet_pton(AF_INET, ipAdress.c_str(), &hint.sin_addr);

    // require connection first
    cout << "Welcome! Use the command /connect to connect to the server.\n";
    do{ 
        getline(cin, userInput);
        if(userInput.compare("/connect")==0){
            // Create socket
            sock = socket(AF_INET, SOCK_STREAM, 0);

            if(sock == -1){
                cerr << "ALERT: Failed at creating socket!" << endl;
                exit(EXIT_FAILURE);
            }
            // Connect to server
            int connectRes = connect(sock, (sockaddr*) &hint, sizeof(hint));

            if(connectRes == -1){
                cerr << "ALERT: Failed to connect to server!" << endl;
                exit(EXIT_FAILURE);
            }           

            cout << "ALERT: Connected to server successfully!" << endl;
            cout << "Choose your nickname now with the command /nickname" << endl;
        }else if(userInput.compare("/quit")==0){
            exit(EXIT_SUCCESS);
        }else if(sock == 0){
            cout << "ALERT: You must first connect to the server!" << endl;
        }
    }while(sock == 0);

    thread th(routine, sock); // keeps geting msgs in the background

    // command loops and messages
    cout << "Use the command /join to join a room!" << endl;
    while(1){
        sleep(0.1);
        getline(cin, userInput);

        if (userInput.compare("/quit") == 0){
            close(sock);
            exit(EXIT_SUCCESS);
        }else{
            if(!send_msg(sock, userInput)){
                cout << "ALERT: Error at sending msg!" << endl;
            }
        }
    }

    return 0;
}