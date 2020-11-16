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


int main(int argc, char* argv[])
{
    int clients_socket, res;
    clients_socket = socket(AF_INET6, SOCK_STREAM, 0);

    if(clients_socket < 0){
        return -1;
    }

    set_server_socket();

    //try to bind the socket to the addres and port number
    res = bind(clients_socket, (struct sockaddr*)&server_addr, sizeof(server_addr));

    if(res < 0){
        close(clients_socket);
        cout << "Error de bind" << endl;
        return -1;
    } else{
        cout << "Binding was done correctly" << endl;
    }

    //try to listen
    res = listen(clients_socket, BACKLOG_CLIENTS);

    if(res < 0){
        close(clients_socket);
        cout << "Error de listen" << endl;
        return -1;
    } else{
        cout << "Listening was done correctly you can now accept conexions" << endl;
    }

    int client;
    client = accept(clients_socket, nullptr, nullptr);
    if(client < 0){
        cout << "Error de accept" << endl;
    } else {
        cout << "Client accepted successfully" << endl;
    }
    //try to send something


}
