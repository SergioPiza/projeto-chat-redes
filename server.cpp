#include <iostream>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <string>

#define PORT 5000  
#define BUFFER_SIZE 4096
#define MAX_CLI 30
#define NICK_SIZE 50
#define CHAN_NAME_SIZE 200
 
// client data
typedef struct client_st{
    int sock;
    char channel[CHAN_NAME_SIZE];
    char nick[NICK_SIZE];
    bool adm;
    bool muted;
}client;

using namespace std;

void change_channel_adm(char[], client[], int, sockaddr_in, int);
void broadcast_msg(char[], client[], int, sockaddr_in, int);

bool send_msg(int sock, char msg[]){
    for(int i=0; i<5; i++){
        int res = send(sock, msg, strlen(msg), 0);
        if(res > 0)
            return true;
    }
    return false;
}

void disconnect_client(client clients[], int pos, sockaddr_in hint, int hintSize){
    getpeername(clients[pos].sock , (sockaddr*) &hint, (socklen_t*) &hintSize);
    cout << "Host Disconnected, ip " << inet_ntoa(hint.sin_addr) << ", port " << ntohs(hint.sin_port) << endl;

    // warn users from the same chat
    if(strcmp(clients[pos].channel, "")!=0){
        char m[BUFFER_SIZE] = "SERVER: the user ";
        strcat(m, clients[pos].nick);
        strcat(m, " has left the channel!");
        broadcast_msg(m, clients, pos, hint, hintSize);
    }

    // if was adm, change adm
    if(clients[pos].adm)
        change_channel_adm(clients[pos].channel, clients, pos, hint, hintSize);
    
    // delete user
    close(clients[pos].sock);
    clients[pos].sock = 0;
    strcpy(clients[pos].channel, "");
    strcpy(clients[pos].nick, "");
    clients[pos].adm = false;
    clients[pos].muted = false;
}

void send_msg_cli(int sock, char msg[], client clients[], int pos, sockaddr_in hint, int hintSize){
    if(!send_msg(sock, msg)){
        disconnect_client(clients, pos, hint, hintSize);
    } 
}

void broadcast_msg(char msg[], client clients[], int pos, sockaddr_in hint, int hintSize){
    for(int i=0; i<MAX_CLI; i++){
        client cli = clients[i];
        if(strcmp(clients[pos].channel, cli.channel) == 0 && i != pos){
            send_msg_cli(cli.sock, msg, clients, i, hint, hintSize);
        } 
    }
}

bool check_channel_name(char chan[]){
    if(chan[0] != '&' && chan[0] != '#')
        return false;
    
    for(int i=1; i < strlen(chan); i++){
        if(chan[i] == ' ' || chan[i] == 7 || chan[i] == ',')
            return false;
    }
    return true;
}

bool check_channel_adm(char chan[], client clients[]){
    for(int i=0; i<MAX_CLI; i++){
        if(strcmp(clients[i].channel, chan) == 0 && clients[i].adm)
            return true;
    }
    return false;
}

void change_channel_adm(char chan[], client clients[], int pos, sockaddr_in hint, int hintSize){
    for(int i=0; i<MAX_CLI; i++){
        if(strcmp(clients[i].channel, chan)==0 && i!=pos){
            clients[i].adm = true;
            char m[] = "SERVER: you are now the adm!";
            send_msg_cli(clients[i].sock, m, clients, i, hint, hintSize);
            break;
        }
    }
}

bool check_nick_name(char nickname[], client clients[]){
    for(int i=0; i<MAX_CLI; i++){
        if(strcmp(clients[i].nick, nickname) == 0)
            return false;
    }
    return true;
}

int find_client(char nick[], client clients[]){
    if(strcmp(nick, "") == 0)
        return -1;
    for(int i=0; i<MAX_CLI; i++){
        if(strcmp(clients[i].nick, "") != 0 && strcmp(clients[i].nick, nick) == 0)
            return i;
    }
    return -1;
}

void adm_commands(int command, char nick[], client clients[], int pos, sockaddr_in hint, int hintSize){
    char m[BUFFER_SIZE];
    
    if(!clients[pos].adm){
        strcpy(m, "SERVER: command exclusive to adms!");
        send_msg_cli(clients[pos].sock, m, clients, pos, hint, hintSize);
        return;
    }

    int i = find_client(nick, clients);
    if(i == -1 || strcmp(clients[pos].channel, clients[i].channel) != 0){
        strcpy(m, "SERVER: Invalid nickname!");
        send_msg_cli(clients[pos].sock, m, clients, pos, hint, hintSize);
        return;
    }

    switch (command){
        case 0: // kick
            if(i == pos){
                char m[] = "SERVER: you cant kick yourself!";
                send_msg_cli(clients[pos].sock, m, clients, pos, hint, hintSize);
                return;
            }
            strcpy(m, "SERVER: Adm kicked you out!");
            send_msg_cli(clients[i].sock, m, clients, i, hint, hintSize);
            disconnect_client(clients, i, hint, hintSize);
            break;
        case 1: // mute
            strcpy(m, "SERVER: Adm muted you!");
            send_msg_cli(clients[i].sock, m, clients, i, hint, hintSize);
            clients[i].muted = true;
            strcpy(m, "SERVER: You muted ");
            strcat(m, clients[i].nick);
            send_msg_cli(clients[pos].sock, m, clients, pos, hint, hintSize);
            break;
        case 2: // unmute
            strcpy(m, "SERVER: Adm unmuted you!");
            send_msg_cli(clients[i].sock, m, clients, i, hint, hintSize);
            clients[i].muted = false;
            strcpy(m, "SERVER: You unmuted ");
            strcat(m, clients[i].nick);
            send_msg_cli(clients[pos].sock, m, clients, pos, hint, hintSize);
            break;
        case 3: // whois
            getpeername(clients[i].sock , (sockaddr*) &hint, (socklen_t*) &hintSize);
            strcpy(m, "SERVER: the ip of ");
            strcat(m, nick);
            strcat(m, " is: ");
            strcat(m, inet_ntoa(hint.sin_addr));
            send_msg_cli(clients[pos].sock, m, clients, pos, hint, hintSize);
            break;
    }
}

int main(int argc, char const *argv[]){
    // Create socket
    int master = socket(AF_INET, SOCK_STREAM, 0); // server sock
    int opt = 1;

    // Check if could create
    if(master == -1){
        cerr << "Failed at create socket!" << endl;
        exit(EXIT_FAILURE);
    }     

    // Allow multiple conn
    if(setsockopt(master, SOL_SOCKET, SO_REUSEADDR, (char *) &opt, sizeof(opt)) == -1){
        cerr << "Multiple con failed" << endl;
        exit(EXIT_FAILURE);
    }

    // Bind socket to IP
    sockaddr_in hint;
    hint.sin_family = AF_INET;
    hint.sin_port = htons(PORT);
    hint.sin_addr.s_addr = INADDR_ANY;
    if(bind(master, (sockaddr*) &hint, sizeof(hint)) == -1){
        cerr << "Failed at binding to IP/port" << endl;     
        exit(EXIT_FAILURE);
    } 

    // Make socket listen
    if(listen(master, SOMAXCONN) == -1){
        cerr << "Can't listen!" << endl;
        exit(EXIT_FAILURE);
    }

    // list of all socks
    fd_set socksFds;
    int hintSize = sizeof(hint);
    
    // create client vector
    client clients[MAX_CLI];
    for(int i=0; i<MAX_CLI; i++){
        clients[i].sock = 0;
        strcpy(clients[i].channel, "");
        strcpy(clients[i].nick, "");
        clients[i].adm = false;
        clients[i].muted = false;
    }

    char buffer[BUFFER_SIZE]; // Buffer to rcv msgs

    // server routine
    while(true){
        // Clear socket set
        FD_ZERO(&socksFds);

        // add server sock to set
        FD_SET(master, &socksFds);
        int max_sd = master;

        // Add children to set
        for(int i=0; i<MAX_CLI; i++){
            client sd = clients[i];

            if(sd.sock) // valid sock
                FD_SET(sd.sock, &socksFds);

            if(sd.sock > max_sd)
                max_sd = sd.sock;
        }

        // Wait for activity 
        int activity = select(max_sd+1, &socksFds, nullptr, nullptr, nullptr);
        
        // error check
        if(activity == -1 && errno != EINTR){
            cerr << "Select Error" << endl;
            continue;
        }

        // If its server activity, then its incoming connection
        if(FD_ISSET(master, &socksFds)){
            // accept conn
            int new_sock = accept(master, (sockaddr*) &hint, (socklen_t*) &hintSize);
            if(new_sock == -1){
                cerr << "Con accept failed!" << endl;
                //exit(EXIT_FAILURE);
                continue;
            }

            // Create a client with the sock
            bool suc = true;
            for(int i=0; i<MAX_CLI; i++){
                if(clients[i].sock == 0){
                    clients[i].sock = new_sock;
                    cout << "sock added" << endl;
                    suc = false;
                    break;
                }
            }
            if(suc){
                char m[] = "SERVER: the server is full, try again later!";
                send_msg(new_sock, m);
            }
        } // IO operation from client
        else{ 
            for(int i=0; i<MAX_CLI; i++){
                client sd = clients[i];
                
                // find client and do operation
                if(FD_ISSET(sd.sock, &socksFds)){
                    // recv msg
                    int bytesRecv = recv(sd.sock, buffer, BUFFER_SIZE, 0);
                    buffer[bytesRecv] = '\0';
                    string sr = string(buffer, bytesRecv);

                    if(bytesRecv == -1){
                        cerr << "Error recv msg" << endl;
                        continue;
                    } else if(bytesRecv == 0){ // Disconnected
                        disconnect_client(clients, i, hint, hintSize);
                    }else{ // analize msg
                        if(sr.compare("/ping") == 0){
                            // send pong
                            char m[] = "SERVER: pong";
                            send_msg_cli(sd.sock, m, clients, i, hint, hintSize);
                        }else if(sr.compare(0, 10, "/nickname ")==0){ // /nickname
                            char aux[NICK_SIZE];
                            sr.copy(aux, sr.size()-10, 10);
                            aux[sr.size()-10] = '\0'; // end string
                            
                            // try to change nick and warn user
                            if(check_nick_name(aux, clients)){
                                strcpy(clients[i].nick, aux);
                                char m[] = "SERVER: nick changed to ";
                                strcat(m, aux);
                                send_msg_cli(sd.sock, m, clients, i, hint, hintSize);
                            }else{
                                char m[] = "SERVER: nickname is already taken, choose another!";
                                send_msg_cli(sd.sock, m, clients, i, hint, hintSize);
                            }
                        }else if(strcmp(sd.nick, "") == 0){
                            // send msg for him to make a nickname
                            char m[] = "SERVER: you have to define a nickname first!";
                            send_msg_cli(sd.sock, m, clients, i, hint, hintSize);
                        }else if(sr.compare(0, 6, "/join ") == 0){ // join channel 
                            char chan[CHAN_NAME_SIZE];
                            sr.copy(chan, sr.size()-6, 6);
                            chan[sr.size()-6] = '\0'; // end string
                            // check channel name 
                            if(check_channel_name(chan)){
                                if(strcmp(chan, sd.channel) == 0){
                                    char m[] = "SERVER: you are already in this channel!";
                                    send_msg_cli(sd.sock, m, clients, i, hint, hintSize);
                                }else{
                                    if(sd.adm)
                                        change_channel_adm(sd.channel, clients, i, hint, hintSize);
                                    if(!check_channel_adm(chan, clients)){
                                        clients[i].adm = true;
                                        char m[] = "SERVER: congratulations, you are the adm!";
                                        send_msg_cli(sd.sock, m, clients, i, hint, hintSize);
                                    }else{
                                        clients[i].adm = false;
                                    }                                       
                                    clients[i].muted = false;

                                    if(strcmp(clients[i].channel, "") != 0){                                
                                        char m[BUFFER_SIZE] = "SERVER: user ";
                                        strcat(m, sd.nick);
                                        strcat(m, " has left the channel!");
                                        broadcast_msg(m, clients, i, hint, hintSize);
                                    }

                                    strcpy(clients[i].channel, chan);       
                                    
                                    char m2[BUFFER_SIZE] = "SERVER: user ";
                                    strcat(m2, sd.nick);
                                    strcat(m2, " has joined the channel!");
                                    broadcast_msg(m2, clients, i, hint, hintSize);
                                }                                
                            }else{
                                char m[] = "SERVER: channel name bad formated. Must start with # or & and have no white spaces!";
                                send_msg_cli(sd.sock, m, clients, i, hint, hintSize);
                            }
                        }else if (strcmp(sd.channel, "") == 0){
                            char m[] = "SERVER: you need to be in a channel!";
                            send_msg_cli(sd.sock, m, clients, i, hint, hintSize);
                        }else if(sr.compare(0, 6, "/kick ") == 0){ // kick
                            char aux[NICK_SIZE];
                            sr.copy(aux, sr.size()-6, 6);
                            aux[sr.size()-6] = '\0';
                            adm_commands(0, aux, clients, i, hint, hintSize);
                        }else if(sr.compare(0, 6, "/mute ") == 0){ // mute
                            char aux[NICK_SIZE];
                            sr.copy(aux, sr.size()-6, 6);
                            aux[sr.size()-6] = '\0';
                            adm_commands(1, aux, clients, i, hint, hintSize);
                        }else if(sr.compare(0, 8, "/unmute ") == 0){ // unmute
                            char aux[NICK_SIZE];
                            sr.copy(aux, sr.size()-8, 8);
                            aux[sr.size()-8] = '\0';
                            adm_commands(2, aux, clients, i, hint, hintSize);
                        }else if(sr.compare(0, 7, "/whois ") == 0){ // whois
                            char aux[NICK_SIZE];
                            sr.copy(aux, sr.size()-7, 7);
                            aux[sr.size()-7] = '\0';
                            adm_commands(3, aux, clients, i, hint, hintSize);
                        }else if(sr[0] == '/'){ // wrong command
                            char m[] = "SERVER: the command is incorrect!";
                            send_msg_cli(sd.sock, m, clients, i, hint, hintSize);
                        }else{ // normal text
                            if(sd.muted){
                                char m[] = "SERVER: you are muted!";
                                send_msg_cli(sd.sock, m, clients, i, hint, hintSize);
                            }else{
                                char m[BUFFER_SIZE+NICK_SIZE+5];
                                strcpy(m, sd.nick);
                                strcat(m, ": ");
                                strcat(m, buffer);
                                broadcast_msg(m, clients, i, hint, hintSize);
                            }
                        }
                    }
                }
            }
        }
    }

    return 0;
}