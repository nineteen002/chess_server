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
#endif

#define SERVER_PORT 4994
#define BACKLOG_CLIENTS 6
using namespace std;

struct sockaddr_in6 server_addr;

void set_server_socket(){
    server_addr.sin6_family = AF_INET6;
    server_addr.sin6_port = htons(SERVER_PORT);
    server_addr.sin6_addr = in6addr_any;
}

int bindClient(int clients_socket){
    int res = bind(clients_socket, (struct sockaddr*)&server_addr, sizeof(server_addr));

    if(res < 0){
        close(clients_socket);
        cout << "Error: Bind could not be done correctly, try again" << endl;
        exit(EXIT_FAILURE);
    } else{
        cout << "Binding was done correctly" << endl;
    }
    return res;
}

int listenForClient(int clients_socket){
    int res = listen(clients_socket, BACKLOG_CLIENTS);

    if(res < 0){
        close(clients_socket);
        cout << "ERROR: Listen command could not be executed, try again" << endl;
        exit(EXIT_FAILURE);
    } else{
        cout << "Listening was done correctly! You can now accept conexions.." << endl;
    }
    return res;
}

int acceptClient(int clients_socket){
    int client = accept(clients_socket, nullptr, nullptr);
    if(client < 0){
        cout << "ERROR: Could not accept client" << clients_socket << endl;
        return -1;
    } else {
        cout << "Client accepted successfully" << endl;
    }
    return client;
}

int main(int argc, char* argv[])
{
    int clients_socket, res;
    clients_socket = socket(AF_INET6, SOCK_STREAM, 0);

    if(clients_socket < 0){
        return -1;
    }

    set_server_socket();

    //try to bind the socket to the addres and port number
    bindClient(clients_socket);

    //try to listen
    listenForClient(clients_socket);

    //try accepting client
    int client = acceptClient(clients_socket);
    //try to read something

    char buffer[1024];

    res = recv(client,buffer, sizeof(buffer),0);
    if(res < 0){
        cout << "Error al recibir informacion" << endl;
    }
    else{
        cout << "Mensaje recibido: " << buffer;
    }

}
