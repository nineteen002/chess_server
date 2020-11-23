#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

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
int numeroSala = 0;

void set_server_socket() {
    listeningPort.sin6_family = AF_INET6;
    listeningPort.sin6_port = htons(SERVER_PORT);
    listeningPort.sin6_addr = in6addr_any;
}

void set_watched_array(int main_socket) {
    watchedElements[0].fd  = main_socket;
    watchedElements[0].events = POLLIN;
    watchedElements[0].revents = 0;
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

void sendDataToClient(int client) {
    //TRY SENDING DATA
    string cadena;
    char buffer[1024];
    //buffer[0] = 1;
    //buffer[1] = 170;
    //buffer[2] = '\r\n';
    cadena = "Welcome to chessworld";
    strcpy(buffer, cadena.c_str());
    send(client, buffer, sizeof(cadena.c_str()),0);
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
    sendDataToClient(new_client);
    cout << "Sending data" << endl;
    return new_client;
}

int readSocket(int client) {
    char buffer[1024];
    int res = recv(client, buffer, sizeof(buffer),0);
    if(res < 0) {
        cout << "ERROR: Could not receive data from client " << client << endl;
        return -1;
    } else if(res > 0) {
        cout << "Data received: " << buffer << endl;
        bzero((char*)&buffer,sizeof(buffer));
        return 1;
    } else if(res == 0) {
        cout << "Se cerro cliente" << client << endl;
        return 0;
    }
}

void closeClientConnection(int client_index){
    cout << "Client that closed connection is " << watchedElements[client_index].fd;
    cout << "Last element on the list" << watchedElements[totalClients-1].fd;
    if(totalClients == 1){
        close(watchedElements[client_index].fd);
        totalClients--;
    } else {
        close(watchedElements[client_index].fd);

        watchedElements[client_index].fd  = watchedElements[totalClients-1].fd;
        watchedElements[client_index].events =  watchedElements[totalClients-1].events;
        watchedElements[client_index].revents = watchedElements[totalClients-1].revents;
    }
}

void addNewClientToWatchedList(int main_socket) {
    acceptClient(main_socket);
    sendDataToClient(watchedElements[totalClients-1].fd);
}

void checkClientListForSomethingToRead() {
    int client;
    int closeClient;
    for(int i = 0; i < totalClients; i++) {
        if((watchedElements[i].revents &POLLIN) != 0) {
            client = watchedElements[i].fd;
            if(readSocket(client) == 0){ //cierre de conexion
                closeClientConnection(i);
            }
            watchedElements[i].revents = 0;
        }
    }
}

int main(int argc, char* argv[]) {
    int main_socket, res, client;

    //START
    set_server_socket();

    //TODO: RESOLUCION DNS

    main_socket = socket(AF_INET6, SOCK_STREAM, 0);
    if(main_socket < 0) {
        cout << "ERROR: Could no create main socket" << endl;
        return -1;
    }

    //try to bind the socket to the addres and port number
    bindClient(main_socket);

    //try to listen
    listenForClient(main_socket);

    while(1) {
        set_watched_array(main_socket);

        res = poll(watchedElements, totalClients, 100);
        if(res < 0) {
            cout << "ERROR: Poll could not be done properly" << endl;
            return -1;
        }

        //ACCEPT CLIENT AND ADD TO WATCHED ELEMENTS
        if(watchedElements[0].revents & POLLIN) {
            cout << "New event happened" << endl;
            addNewClientToWatchedList(main_socket);
        }

        //CHECK EACH ELEMENT IN LIST TO SEE IF THERE IS SOMETHING TO READ
        checkClientListForSomethingToRead();

        //TO DO: REMOVE ELEMENT FROM ARRAY WHEN CONEXION CLOSES2
    }

    close(client);
    close(main_socket);
    return 0;
}


