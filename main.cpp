#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

#ifdef __WIN32__
#include <winsock.h>
#include <winsock2.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/poll.h>
#endif

#define SERVER_PORT 4994
#define BACKLOG_CLIENTS 6

using namespace std;

struct sockaddr_in6 listeningPort;
struct pollfd watchedElements[6];
int totalClients = 1;

void set_server_socket() {
    listeningPort.sin6_family = AF_INET6;
    listeningPort.sin6_port = htons(SERVER_PORT);
    listeningPort.sin6_addr = in6addr_any;
}

int bindClient(int socket) {
    int res = bind(socket, (struct sockaddr*)&listeningPort, sizeof(listeningPort));

    if(res < 0) {
        close(socket);
        cout << "Error: Bind could not be done correctly, try again" << endl;
        exit(EXIT_FAILURE);
    } else {
        cout << "Binding was done correctly" << endl;
    }
    return res;
}

int listenForClient(int socket) {
    int res = listen(socket, BACKLOG_CLIENTS);

    if(res < 0) {
        close(socket);
        cout << "ERROR: Listen command could not be executed, try again" << endl;
        exit(EXIT_FAILURE);
    } else {
        cout << "Listening was done correctly! You can now accept conexions..." << endl;
    }
    return res;
}

int acceptClient(int socket) {
    int new_client = accept(socket, nullptr, nullptr);

    if(new_client < 0) {
        cout << "ERROR: Could not accept client" << socket << endl;
        return -1;
    }

    watchedElements[totalClients].fd  = new_client;
    watchedElements[totalClients].events = POLLIN;
    watchedElements[totalClients].revents = 0;
    totalClients++;

    cout << "New client " << new_client << " accepted successfully and added to list" << endl;

    return new_client;
}

void readSocket(int client) {
    char buffer[1024];
    int res = recv(client, buffer, sizeof(buffer),0);
    if(res < 0) {
        cout << "ERROR: No se pudo leer o no hay nada en el buffer" << endl;
    } else {
        cout << "Mensaje del cliente " << client << " recibido: " << buffer << endl;
    }
}

int main(int argc, char* argv[]) {
    int main_socket, res, client;

    //START
    set_server_socket();

    main_socket = socket(AF_INET6, SOCK_STREAM, 0);
    if(main_socket < 0) {
        cout << "ERROR: Could no create main socket" << endl;
        return -1;
    }

    //try to bind the socket to the addres and port number
    bindClient(main_socket);

    //try to listen
    listenForClient(main_socket);

    int g = 0;
    while(1) {
        watchedElements[0].fd  = main_socket;
        watchedElements[0].events = POLLIN;
        watchedElements[0].revents = 0;

        res = poll(watchedElements, totalClients, 100);

        if(res < 0) {
            cout << "ERROR" << endl;
            return -1;
        }

        if(watchedElements[0].revents & POLLIN){
            acceptClient(main_socket);
        }

        /*
        for(int i = 0; i < totalClients; i++){
            //cout << "I = " << i;
            //cout << " Client: " << watchedElements[i].fd;
            //cout << " ,Events: " << watchedElements[i].events;
            //cout << " ,Revents: " << watchedElements[i].revents << endl;

            if((watchedElements[i].revents &POLLIN) != 0){
                //cout << "Inside revents &Pollin != 0" << endl;
                client = watchedElements[i].fd;

                readSocket(client);

                watchedElements[i].revents = 0;
                //cout << " ,Revents: " << watchedElements[i].revents << endl;

            }
            g++;
        }
        */
    }

    close(client);
    close(main_socket);
}
