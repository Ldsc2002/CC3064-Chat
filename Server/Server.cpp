#include <iostream>
#include <unistd.h>
#include <pthread.h>
#include <netinet/in.h>
#include <sys/socket.h>

void* clientHandler(void* arg) {
    pthread_t thisThread = pthread_self();
    int clientSocket = *(int *) arg;

    printf("Thread %lu is handling client\n", thisThread);

    char buffer[1024] = {0};
    int readResult;
    
    while (true) {
        readResult = read(clientSocket, buffer, 1024);
        if (readResult < 0) {
            printf("Error reading from socket\n");
            pthread_exit(NULL);
        }

        printf("%lu --- Message received from client: %s\n", thisThread, buffer);
    }

    pthread_exit(NULL);
}

int main() {
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