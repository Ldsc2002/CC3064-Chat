#include <iostream>
#include <unistd.h>
#include <pthread.h>
#include <netinet/in.h>
#include <arpa/inet.h>  
#include <sys/socket.h>
#include <cstring>

using std::string;

string getIP() {
    const char* dnsServer = "8.8.8.8";
    int dnsPort = 53;
    struct sockaddr_in serv;
    int sock = socket(AF_INET, SOCK_DGRAM, 0);

    if(sock < 0) {
        close(sock);
        return "";
    }

    memset(&serv, 0, sizeof(serv));
    serv.sin_family = AF_INET;
    serv.sin_addr.s_addr = inet_addr(dnsServer);
    serv.sin_port = htons(dnsPort);

    int err = connect(sock, (const struct sockaddr*)&serv, sizeof(serv));
    if (err < 0) {
        close(sock);
        return "";
    }

    struct sockaddr_in name;
    socklen_t namelen = sizeof(name);
    err = getsockname(sock, (struct sockaddr*)&name, &namelen);

    char buffer[80];
    const char* p = inet_ntop(AF_INET, &name.sin_addr, buffer, 80);
    
    if(p != NULL) {
        close(sock);
        string res = (string)buffer;
        return res;
    } else {
        close(sock);
        return "";
    }
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
    
    char buffer[1024] = {0};

    while (true) {
        buffer[1024] = {0};

        printf("Enter enter email address: ");
        scanf("%s", buffer);

        bool hasAt = false;
        for (int i = 0; i < strlen(buffer); i++) {
            if (buffer[i] == '@') {
                hasAt = true;
                break;
            }
        }

        if (hasAt) {
            break;
        } else {
            printf("Invalid email address\n");
        }
    }

    string ip = getIP();

    printf("Client IP: %s\n", ip.c_str());
    printf("Email: %s\n", buffer);

    while (true) {
        printf("1. Open global chat\n");
        printf("2. Open private chat\n");
        printf("3. Exit\n");

        int choice;
        scanf("%d", &choice);

        switch (choice) {
            case 1: {
                int pid = fork();
                bool running = true;

                if (pid == 0) {
                    // while (running) {
                    //     char buffer[1024] = {0};
                    //     int readResult = read(serverSocket, buffer, 1024);

                    //     printf("Message: %s\n", buffer);
                    // }
                } else {
                    while (running) {
                        char buffer[1024] = {0};
                        printf("Enter message ('exit' to return to menu): ");
                        scanf("%s", buffer);

                        if (strcmp(buffer, "exit") == 0) {
                            running = false;
                        }

                        send(serverSocket, buffer, strlen(buffer), 0);
                    }

                    wait(NULL);
                }
            }
            case 2: {
                printf("Opening private chat\n");
                break;
            }
            case 3: {
                printf("Exiting\n");
                break;
            }
            default: {
                printf("Invalid choice\n");
                break;
            }
        }
    }

    close(serverSocket);
    shutdown(serverSocket, SHUT_RDWR);

    printf("Client is shutting down\n");
    return 0;
}