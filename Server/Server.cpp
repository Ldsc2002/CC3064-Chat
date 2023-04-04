#include <iostream>
#include <unistd.h>
#include <pthread.h>
#include <netinet/in.h>
#include <cstring>
#include <sys/socket.h>
#include <wait.h>
#include "project.pb.h"

using std::string;

struct Client {
    string username;
    string ip;
    int socket;
    bool online;
};

Client clients[100] = {};  

int getFirstEmptySlot() {
    for (int i = 0; i < 100; i++) {
        if (clients[i].username == "") {
            return i;
        }
    }

    return -1;
}

bool checkIfUserExists(string ip) {
    for (int i = 0; i < 100; i++) {
        if (clients[i].ip == ip) {
            return true;
        }
    }

    return false;
}

void* clientHandler(void* arg) {
    pthread_t thisThread = pthread_self();
    int clientSlot = -1;
    int clientSocket = *(int *) arg;

    printf("Thread %lu is handling client\n", thisThread);

    char buffer[1024] = {0};
    int readResult;
    
    while (true) {
        bool noHeartbeat = false;
        int pid = fork();
        
        if (pid == 0) {
            int timeInactive = 0;

            while (true) {
                sleep(10);                
                timeInactive += 10;
                printf("Thread %lu: No heartbeat received - %d seconds\n", thisThread, timeInactive);

                if (timeInactive >= 60) {
                    close(clientSocket);
                    noHeartbeat = true;
                    break;
                }
            }

        } else if (pid > 0) {
            readResult = read(clientSocket, buffer, 1024);
            kill(pid, SIGKILL); 
            wait(NULL);
        } 

        if (noHeartbeat) {
            printf("Thread %lu: Client disconnected due to inactivity\n", thisThread);
            break;
        }

        if (readResult < 0) {
            printf("Thread %lu: Error reading from socket\n", thisThread);
            break;

        } else if (readResult == 0) {
            printf("Thread %lu: Client disconnected\n", thisThread);

            if (clientSlot != -1) {
                clients[clientSlot].username = "";
                clients[clientSlot].ip = "";
                clients[clientSlot].socket = 0;
                clients[clientSlot].online = false;
            }

            break;

        } else {
            chat::UserRequest newRequest;
            newRequest.ParseFromString((string)buffer);

            if (newRequest.option() == 1) {
                // User registration
                printf("Thread %lu: User %s wants to register with IP %s\n", thisThread, newRequest.mutable_newuser() -> username().c_str(), newRequest.mutable_newuser() -> ip().c_str());

                chat::ServerResponse newResponse;
                newResponse.set_option(1);
                newResponse.set_code(400);
                newResponse.set_servermessage("Error registering user");

                if (checkIfUserExists(newRequest.mutable_newuser() -> ip())) {
                    printf("Thread %lu: User with IP %s already exists\n", thisThread, newRequest.mutable_newuser() -> ip().c_str());
                    newResponse.set_servermessage("User with this IP already exists");

                } else {
                    clientSlot = getFirstEmptySlot();

                    printf("Thread %lu: Client slot: %d\n", thisThread, clientSlot);

                    if (clientSlot == -1) {
                        printf("Thread %lu: No empty slots\n", thisThread);
                        break;
                    }

                    clients[clientSlot].username = newRequest.mutable_newuser() -> username();
                    clients[clientSlot].ip = newRequest.mutable_newuser() -> ip();
                    clients[clientSlot].socket = clientSocket;
                    clients[clientSlot].online = true;

                    newResponse.set_code(200);
                    newResponse.set_servermessage("User registered");

                    printf("Thread %lu: User %s registered with IP %s\n", thisThread, clients[clientSlot].username.c_str(), clients[clientSlot].ip.c_str());
                }

                string responseString;
                newResponse.SerializeToString(&responseString);

                send(clientSocket, responseString.c_str(), responseString.length(), 0);

            } else if (newRequest.option() == 2) {
                // User information request
            } else if (newRequest.option() == 3) {
                // Status change
            } else if (newRequest.option() == 4) {
                // New message
            } else if (newRequest.option() == 5) {
                // Client hearbeat
            } else {
                printf("Thread %lu: Unknown request\n", thisThread);
            }
        }
    }

    pthread_exit(NULL);
}

int main() {
    GOOGLE_PROTOBUF_VERIFY_VERSION;

    struct sockaddr_in serverAddress;
    int serverSocket;
    int serverPort;

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        printf("Error creating socket\n");
        return 1;
    }

    serverPort = setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &serverPort, sizeof(serverPort));
    if (serverPort < 0) {
        printf("Error setting socket options\n");
        return 1;
    }

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(8080);

    int bindResult = bind(serverSocket, (struct sockaddr *) &serverAddress, sizeof(serverAddress));
    if (bindResult < 0) {
        printf("Error binding socket\n");
        return 1;
    }

    if (listen(serverSocket, 5) < 0) {
        printf("Error listening to socket\n");
        return 1;
    }

    while (true) {
        printf("Waiting for client to connect...\n");
            
        int size = sizeof(serverAddress);
        int newSocket = accept(serverSocket, (struct sockaddr *) &serverAddress, (socklen_t *) &size);        
        
        if (newSocket < 0) {
            printf("Error accepting connection\n");
            return 1;
        }

        pthread_t thread;
        pthread_create(&thread, NULL, &clientHandler, (void *) &newSocket);
    }

    close(serverSocket);
    shutdown(serverSocket, SHUT_RDWR);

    printf("Server is shutting down\n");
    return 0;
}