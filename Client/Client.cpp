#include <iostream>
#include <unistd.h>
#include <pthread.h>
#include <netinet/in.h>
#include <arpa/inet.h>  
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

    send(serverSocket, "Hello from client", 17, 0);
    printf("Message sent to server\n");

    char buffer[1024] = {0};
    int connectionData = read(serverSocket, buffer, 1024);
    if (connectionData < 0) {
        printf("Error reading from socket\n");
        return 1;
    }

    printf("%s\n", buffer);

    close(serverSocket);
    shutdown(serverSocket, SHUT_RDWR);

    printf("Client is shutting down\n");
    return 0;
}