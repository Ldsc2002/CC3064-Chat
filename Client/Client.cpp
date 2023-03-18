#include <iostream>
#include <unistd.h>
#include <pthread.h>
#include <netinet/in.h>
#include <arpa/inet.h>  
#include <sys/socket.h>
#include <cstring>

int main() {
    struct sockaddr_in serverAddress;
    int serverSocket;
    int serverPort;

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        printf("Error creating socket\n");
        return 1;
    }

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(8080);

    if (inet_pton(AF_INET, "127.0.0.1", &serverAddress.sin_addr) <= 0) {
        printf("Error converting address\n");
        return 1;
    }

    int connectResult = connect(serverSocket, (struct sockaddr *) &serverAddress, sizeof(serverAddress));
    if (connectResult < 0) {
        printf("Error connecting to server\n");
        return 1;
    }

    while (true) {
        char buffer[1024] = {0};
        printf("Enter message to send to server ('exit' to quit): ");
        scanf("%s", buffer);

        if (strcmp(buffer, "exit") == 0) {
            break;
        }

        send(serverSocket, buffer, strlen(buffer), 0);
    }

    close(serverSocket);
    shutdown(serverSocket, SHUT_RDWR);

    printf("Client is shutting down\n");
    return 0;
}