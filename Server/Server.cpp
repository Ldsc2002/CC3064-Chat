#include <iostream>
#include <unistd.h>
#include <pthread.h>
#include <netinet/in.h>
#include <sys/socket.h>

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

    int newSocket = accept(serverSocket, (struct sockaddr *) &serverAddress, (socklen_t *) &serverAddress);
    if (newSocket < 0) {
        printf("Error accepting connection\n");
        return 1;
    }

    char buffer[1024] = {0};
    int connectionData = read(newSocket, buffer, 1024);
    if (connectionData < 0) {
        printf("Error reading from socket\n");
        return 1;
    }

    printf("%s\n", buffer);

    send(newSocket, "Hello from server", 17, 0);
    printf("Message sent to client\n");

    close(serverSocket);
    shutdown(serverSocket, SHUT_RDWR);

    printf("Server is shutting down\n");
    return 0;
}