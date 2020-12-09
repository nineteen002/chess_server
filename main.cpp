#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <list>

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
struct pollfd watchedElements[BACKLOG_CLIENTS+1];
int totalClients = 1;
int historyClient = 0;
int numeroSala = 0;
int historySala = 1;

enum {
    STATE_INITIAL_CONNECTION = 0,
    STATE_WAITING_IN_GAME,
    STATE_PLAYING_GAME,
    STATE_GAME_ENDED
};

class Client;
class Game;

class Client{
    public:
        int fd;
        int name_length;
        char username[100]; 
        int state;
        int team;
        Game* sala;
};

class Game{
    public:
        Client* white;
        Client* black;
        int num_sala;
};

Client* clientList[BACKLOG_CLIENTS+1];
Game* salas[BACKLOG_CLIENTS/2];

//Setup
void setServerSocket();
void initializeLists();
void setWatchedArray(int);
int bindClient(int);
int listenForClient(int);

//To send
void sendDataToClient(int, char*);

//Add new client
void addNewClientToWatchedList(int);
int acceptClient(int);
void addClient(int);
void addClientToSala(Client*);
void addWhiteToNewGame(Client*);
void addBlackToExistingGame(Client*);

//To Read
void checkClientListForSomethingToRead();
int readSocket(Client*);
void processDataRecieved(string, Client*);

//Close conection
void closeClientConnection(int);
void closeOpponentConnection(Client*);
void closeGameConnection(Game*);

//Search functions
Game* searchForClientsGame(int);
int searchForClientInList(int fd);
int searchForSalaInList(Game*);

//PackagesssEsesesss
void conectionToGame(Client*);
void startGame(Client*, Client*);
void readUsernamePackage(Client*, string);
void readChesspieceMovement(Client*, string);
void sendChesspieceMovement(Client*, int, int);
void readChatMessage(Client*, string);
void bounceRematch(Client*, string);

int main(int argc, char* argv[]) {
    int main_socket, res, client;

    //START
    setServerSocket();
    initializeLists();

    //TODO: RESOLUCION DNS

    main_socket = socket(AF_INET6, SOCK_STREAM, 0);
    if(main_socket < 0) {
        cout << "ERROR: Could no create main socket" << endl;
        return -1;
    }

    //try to bind socket
    bindClient(main_socket);

    //try to listen
    listenForClient(main_socket);

    while(1) {
        setWatchedArray(main_socket);

        res = poll(watchedElements, totalClients, 100);
        if(res < 0) {
            cout << "ERROR: Poll could not be done properly" << endl;
            return -1;
        }

        //ACCEPT CLIENT AND ADD TO WATCHED ELEMENTS
        if(watchedElements[0].revents & POLLIN) {
            cout << "NEW event happened" << endl;
            addNewClientToWatchedList(main_socket);
            cout << "TOTAL clients: " << totalClients-1 << endl;
        }

        //CHECK EACH ELEMENT IN LIST TO SEE IF THERE IS SOMETHING TO READ
        checkClientListForSomethingToRead();

        //TO DO: REMOVE ELEMENT FROM ARRAY WHEN CONEXION CLOSES2
    }

    close(client);
    close(main_socket);
    return 0;
}

void setServerSocket() {
    listeningPort.sin6_family = AF_INET6;
    listeningPort.sin6_port = htons(SERVER_PORT);
    listeningPort.sin6_addr = in6addr_any;
}

void initializeLists() {
    for(int i = 1; i < BACKLOG_CLIENTS+1; i++){
        clientList[i] = nullptr;
    }
    for(int i = 0; i < BACKLOG_CLIENTS/2; i++){
        salas[i] = nullptr;
    }
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

void setWatchedArray(int main_socket) {
    watchedElements[0].fd  = main_socket;
    watchedElements[0].events = POLLIN;
    watchedElements[0].revents = 0;

    Client* new_client_connection = new Client();
    new_client_connection->fd = main_socket;
    new_client_connection->username[0] = 0;
    //estado

    clientList[0] = new_client_connection;
}

void addNewClientToWatchedList(int main_socket) {
    acceptClient(main_socket);
}

int acceptClient(int socket) {
    //CAN ACCEPT MORE CLIENTS?
    if(totalClients >= BACKLOG_CLIENTS){
        cout << "ERROR: Can not accept more clients, server is full" << endl;
        return -1;
    }

    int new_client = accept(socket, nullptr, nullptr);

    if(new_client < 0) {
        cout << "ERROR: Could not accept client" << socket << endl;
        return -1;
    }

    addClient(new_client);

    return new_client;
}

void addClient(int new_client){
    Client* new_client_connection = new Client();

    watchedElements[totalClients].fd  = new_client;
    watchedElements[totalClients].events = POLLIN;
    watchedElements[totalClients].revents = 0;
    
    new_client_connection->fd = new_client;
    new_client_connection->username[0] = 0;
    new_client_connection->name_length = 0;
    new_client_connection->team = -1;
    new_client_connection->state = STATE_INITIAL_CONNECTION;

    clientList[totalClients] = new_client_connection;
    cout << "---> NEW CLIENT: " << new_client << " accepted..." << endl;

    totalClients++;
    historyClient++;
}

void addClientToSala(Client* c) {  
    if(((totalClients)%2) == 0) {
        addWhiteToNewGame(c);
        conectionToGame(c);
    } else {
        addBlackToExistingGame(c);
        conectionToGame(c);
        startGame(c->sala->white, c->sala->black);
        startGame(c->sala->black, c->sala->white);
    }
}

void addWhiteToNewGame(Client* c){
    Game* sala = new Game();

    sala->white = c;
    sala->white->team = 1;
    sala->white->state = STATE_WAITING_IN_GAME;

    sala->num_sala = historySala;
    sala->black = nullptr;

    salas[numeroSala] = sala;
    c->sala = sala;
    cout << "<<< NEW GAME: " << sala->num_sala << " and adding client " << c->fd << ">>>" << endl;
}

void addBlackToExistingGame(Client* c){
    salas[numeroSala]->black = c;
    salas[numeroSala]->white->state = STATE_PLAYING_GAME;
    salas[numeroSala]->black->state = STATE_PLAYING_GAME;
    salas[numeroSala]->black->team = 0;

    c->sala = salas[numeroSala];

    cout << " <<< EXISTING GAME: " << salas[numeroSala]->num_sala  << " adding client " << c->fd << " >>>" << endl;
    numeroSala++;
    historySala++;
}

void sendDataToClient(int client, char* buffer) {
    //TRY SENDING DATA
    cout <<"SENDING data:" << buffer << " to client " << client << endl;
    int size_buffer;
    if(buffer[0] == 1){
        size_buffer = 3;
    }
    else if (buffer[0] == 2){
        size_buffer = 2 + int(buffer[1]);
        cout << size_buffer << endl;
    }
    else if (buffer[0] == 4){
        size_buffer = 3;
    }
    else if(buffer[0] == 5){
        size_buffer = 2;
    }
    else if(buffer[0] == 9){
        size_buffer = 2 + int(buffer[1]);
        cout << "Package message size: " << size_buffer << endl;
    }

    send(client, buffer, size_buffer, 0);
}

void checkClientListForSomethingToRead() {
    //cout << "Checking list for something to read" << endl;
    Client* client = new Client();
    int closeClient;
    for(int i = 0; i < totalClients; i++) {
        if((watchedElements[i].revents &POLLIN) != 0) {
            client = clientList[i];
            if(readSocket(client) == 0) { //cierre de conexion
                closeClientConnection(i);
            }
            watchedElements[i].revents = 0;
        }
    }
}

int readSocket(Client* client) {
    int buffer_size = 1024;
    char *buffer = new char[buffer_size];

    int res = recv(client->fd, buffer, buffer_size, 0);
    string buffer1 = buffer;
    
    if(res < 0) {
        cout << "ERROR: Could not receive data from client " << client->fd << endl;
        return -1;
    } else if(res > 0) {
        cout << "Data received: " << buffer << endl;
        buffer1 = buffer;
        processDataRecieved(buffer1, client);

        bzero((char*)&buffer,sizeof(buffer));
        return 1;
    } else if(res == 0) {
        return 0;
    }
}

void processDataRecieved(string buffer, Client* client){
    char newBuffer[1024];
    strcpy(newBuffer, buffer.c_str());

    char type_package = newBuffer[0];
    //FIX REMOVE ''
    if(type_package == '0'){ //READ USERNAME 
        readUsernamePackage(client, buffer);
        addClientToSala(client);
    }
    if(int(type_package) == 3){
        readChesspieceMovement(client, buffer);
    }
    if(int(type_package) == 5){
        bounceRematch(client, buffer);
    }
    if(int(type_package) == 9){
        readChatMessage(client, buffer);
    }
}

void closeClientConnection(int client_index) {
    Game* sala = new Game();
    Client* oponent = new Client();

    if((totalClients-1) % 2 == 1) {
        cout << "Closed connection without oponent" << endl;
        close(watchedElements[client_index].fd);
        totalClients--;
        cout << "--x CLOSED connection of client " << watchedElements[client_index].fd << endl;

        int sala_index = searchForSalaInList(clientList[client_index]->sala);
        cout << "Numero de sala " << numeroSala << endl;
        delete salas[sala_index];
        salas[sala_index] = nullptr;
    } 
    else {
        sala = searchForClientsGame(watchedElements[client_index].fd);

        close(watchedElements[client_index].fd);
        cout << "--x CLOSED connection of client " << watchedElements[client_index].fd << endl;

        if(sala->white == clientList[client_index]){
            oponent = sala->black;
        } else{
            oponent = sala->white;
        }

        watchedElements[client_index].fd  = watchedElements[totalClients-1].fd;
        watchedElements[client_index].events =  watchedElements[totalClients-1].events;
        watchedElements[client_index].revents = watchedElements[totalClients-1].revents;
        delete clientList[client_index];
        clientList[client_index] = clientList[totalClients-1];

        totalClients--;
        closeOpponentConnection(oponent);
        closeGameConnection(sala);

    }
}

void closeOpponentConnection(Client* oponent){
    int oponent_index = searchForClientInList(oponent->fd);
    
    char* buffer = "Your oponent left.. closing connection";
    sendDataToClient(oponent->fd, buffer);
    if((totalClients-1) % 2 == 1) {
        close(watchedElements[oponent_index].fd);
    } 
    else{
        watchedElements[oponent_index].fd  = watchedElements[totalClients-1].fd;
        watchedElements[oponent_index].events =  watchedElements[totalClients-1].events;
        watchedElements[oponent_index].revents = watchedElements[totalClients-1].revents;
    }

    cout << "--x CLOSED connection to opponent " << oponent->fd << endl;
    
    totalClients--;
    delete clientList[oponent_index];
    clientList[oponent_index] = clientList[totalClients-1];
}

void closeGameConnection(Game* sala) {
    int num_sala = sala->num_sala;
    int sala_index = searchForSalaInList(sala);

    delete salas[sala_index];
    salas[sala_index] = nullptr;
    
    if(sala_index != 0){
        salas[sala_index] = salas[numeroSala];
    }
    numeroSala--;
    cout << "xxx CLOSED GAME " << num_sala << " xxx"<< endl;
}

Game* searchForClientsGame(int c){
    for(int i = 0; i < numeroSala; i++){
        if(salas[i]->white->fd == c || salas[i]->black->fd == c){
            return salas[i];
        }
    }
    return nullptr;
}

int searchForClientInList(int fd){
    for(int i = 1; i < totalClients; i++){
        if(clientList[i]->fd == fd){
            return i; 
        }
    }
}

int searchForSalaInList(Game* sala){
    for(int i = 0; i < numeroSala; i++){
        if(salas[i]->num_sala == sala->num_sala){
            return i;
        }
    }
}

//Packages
void readUsernamePackage(Client* client, string buffer){
    cout << "READING package username" << endl;
    char newBuffer[1024];
    strcpy(newBuffer, buffer.c_str());

    int lengthOfName = int(newBuffer[1]);

    for(int i = 2; i < 2+lengthOfName; i++){
        client->username[i-2] += buffer[i];
    }
    client->name_length = lengthOfName;

    cout << "Length of name: " << client->name_length << " Name: " << client->username << endl;
}

void readChesspieceMovement(Client* client, string str_buffer){
    cout << "READING package chesspiece movement" << endl;
    char buffer[3];
    strcpy(buffer, str_buffer.c_str());

    int chesspiece = int(buffer[1]);
    int movement = int(buffer[2]);

    cout << "Chesspiece current position: " << chesspiece << " Moves to: " << movement << endl;
    sendChesspieceMovement(client, chesspiece, movement);
}

void sendChesspieceMovement(Client* client_moved, int chesspiece, int movement){
    int opponent_fd;
    char buffer[3];
    if(client_moved->team == 1){
        opponent_fd = client_moved->sala->black->fd;
    }
    else{
        opponent_fd = client_moved->sala->white->fd;
    }
    buffer[0] = 4;
    buffer[1] = chesspiece;
    buffer[2] = movement;
    sendDataToClient(opponent_fd, buffer);
}

void conectionToGame(Client* client){
    char package[3];
    package[0] = 1; //Type
    package[1] = client->sala->num_sala; //numero sala
    
    //white or black
    if(client->team == 1){
        package[2] = 1;
    }
    else{
        package[2] = 0;
    }
    cout << "Package type: " << int(package[0]) << " num_sala " << int(package[1]) << ", white/black " << int(package[2]) << endl;
    sendDataToClient(client->fd, package);
}

void startGame(Client* client, Client* oponent){
    cout << "Starting game package" << endl;
    int len_name = oponent->name_length;
    char package[3+len_name];

    package[0] = 2; //Type
    package[1] = oponent->name_length; //oponents name length

    for(int i = 0; i < oponent->name_length; i++){
        package[2+i] = oponent->username[i]; 
    }

    cout << "Sending to " << client->username << ". Opponent information sent: Length: " << int(package[1]) << " name: " << oponent->username << endl;
    sendDataToClient(client->fd, package);
}

void readChatMessage(Client* client, string str_buffer){
    cout << "READING package chat message" << endl;
    char buffer[1024];
    char chat[254];
    strcpy(buffer, str_buffer.c_str());

    int lengthOfMsg = int(buffer[1]);

    for(int i = 2; i < 2+lengthOfMsg; i++){
        chat[i-2] += buffer[i];
    }

    cout << "Length of msg: " << lengthOfMsg << " Message: " << chat << endl;

    if(client->team == 1 && client->sala->black != nullptr){
        sendDataToClient(client->sala->black->fd, buffer);
    } else if (client->team == 0 && client->sala->white != nullptr) {
        sendDataToClient(client->sala->white->fd, buffer);
    }
}

void bounceRematch(Client* client, string str_buffer){
    cout << "READING package rematch" << endl;
    char buffer[2];
    strcpy(buffer, str_buffer.c_str());

    int rematch = int(buffer[1]);
    if(rematch == 1){
        cout << "Client " << client->fd  << " accepted rematch" << endl;
    } else{
         cout << "Client " << client->fd  << " declined rematch" << endl;
    }

    if(client->team == 1 && client->sala->black != nullptr){
        sendDataToClient(client->sala->black->fd, buffer);
    } else if (client->team == 0 && client->sala->white != nullptr) {
        sendDataToClient(client->sala->white->fd, buffer);
    }
}
