#include <iostream>
#include <unistd.h>
#include <pthread.h>
#include <netinet/in.h>
#include <arpa/inet.h>  
#include <sys/socket.h>
#include <cstring>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <fcntl.h>
#include "project.pb.h"

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

int main(int argc, char** argv) {
    if (argc != 3) {
        printf("Usage: ./Client <IP> <PORT>\n");
        return 1;
    }

    string serverIP = (string)argv[1];
    int serverPort = atoi(argv[2]);
    
    GOOGLE_PROTOBUF_VERIFY_VERSION;

    struct sockaddr_in serverAddress;
    int serverSocket;

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        printf("Error creating socket\n");
        return 1;
    }

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(serverPort);

    if (inet_pton(AF_INET, serverIP.c_str(), &serverAddress.sin_addr) <= 0) {
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
    string username = (string)buffer;

    printf("Client IP: %s\n", ip.c_str());
    printf("Email: %s\n", buffer);

    chat::UserRequest newRegister;

    newRegister.set_option(1);
    newRegister.mutable_newuser() -> set_username(buffer);
    newRegister.mutable_newuser() -> set_ip(ip);

    string serialized;
    newRegister.SerializeToString(&serialized);

    send(serverSocket, serialized.c_str(), serialized.length(), 0);

    int readResult = read(serverSocket, buffer, 1024);
    if (readResult < 0) {
        printf("Error reading from server\n");
        return 1;
    }

    chat::ServerResponse response;
    response.ParseFromString((string)buffer);

    if (response.code() == 200) {
        printf("Successfully registered\n");
    } else {
        printf("Error registering: %s\n", response.servermessage().c_str());
        return 1;
    }

    int flags = fcntl(serverSocket, F_GETFL, 0);
    fcntl(serverSocket, F_SETFL, flags | O_NONBLOCK);

    bool* running = (bool*)mmap(NULL, sizeof(bool), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    *running = true;

    int pid = fork();

    if (pid == 0) {
        int sleepTime = 0;

        while (*running) {
            sleep(1);
            sleepTime++;

            if (sleepTime == 20) {
                chat::UserRequest heartbeat;
                heartbeat.set_option(5);

                string serializedHeartbeat;
                heartbeat.SerializeToString(&serializedHeartbeat);

                send(serverSocket, serializedHeartbeat.c_str(), serializedHeartbeat.length(), 0);
                sleepTime = 0;
            }
        }

        return 0;

    } else if (pid > 0) {
        int pid2 = fork();

        if (pid2 == 0) {
            while (*running) {
                readResult = -1;

                while(readResult == -1 && *running) {
                    readResult = read(serverSocket, buffer, 1024);
                }

                if (readResult < 0 && *running) {
                    printf("Error reading from server\n");
                    return 1;
                }

                chat::ServerResponse response;
                response.ParseFromString((string)buffer);

                if (response.code() == 200) {
                    if (response.option() == 2) {
                        printf("Success: %s\n", response.servermessage().c_str());
                        for (int i = 0; i < response.mutable_connectedusers() -> connectedusers_size(); i++) {
                            printf("%s\n", response.mutable_connectedusers() -> connectedusers(i).username().c_str());
                        }
                    } else if (response.option() == 4) {
                        if (response.has_message()) {
                            if (response.mutable_message() -> message_type() == true) {
                                printf("Public message from %s: %s\n", response.mutable_message() -> sender().c_str(), response.mutable_message() -> message().c_str());
                            } else {
                                printf("Private message from %s: %s\n", response.mutable_message() -> sender().c_str(), response.mutable_message() -> message().c_str());
                            }
                        } else {
                            printf("Success: %s\n", response.servermessage().c_str());
                        }
                    }
                } else {
                    printf("Error: %s\n", response.servermessage().c_str());
                }
            }

            return 0;

        } else if (pid2 > 0) {
            while (*running) {
                printf("1. Send private message\n");
                printf("2. Send public message\n");
                printf("3. Change status\n");
                printf("4. Get list of users\n");
                printf("5. Get information about user\n");
                printf("6. Help\n");
                printf("7. Exit\n");

                int choice;
                scanf("%d", &choice);

                switch (choice) {
                    case 1: {
                        string recipient;
                        string message;

                        printf("Enter recipient: ");
                        scanf("%s", buffer);
                        recipient = (string)buffer;

                        printf("Enter message: ");
                        scanf("%s", buffer);
                        message = (string)buffer;

                        chat::UserRequest privateMessage;
                        privateMessage.set_option(4);
                        privateMessage.mutable_message() -> set_recipient(recipient);
                        privateMessage.mutable_message() -> set_message(message);
                        privateMessage.mutable_message() -> set_message_type(false);
                        privateMessage.mutable_message() -> set_sender(username);

                        string serialized;
                        privateMessage.SerializeToString(&serialized);

                        send(serverSocket, serialized.c_str(), serialized.length(), 0);

                        break;
                    }

                    case 2: {
                        string message;

                        printf("Enter message: ");
                        scanf("%s", buffer);
                        message = (string)buffer;

                        chat::UserRequest publicMessage;
                        publicMessage.set_option(4);
                        publicMessage.mutable_message() -> set_message(message);
                        publicMessage.mutable_message() -> set_message_type(true);
                        publicMessage.mutable_message() -> set_sender(username);

                        string serialized;
                        publicMessage.SerializeToString(&serialized);

                        send(serverSocket, serialized.c_str(), serialized.length(), 0);

                        break;
                    }

                    case 3: {
                        break;
                    }

                    case 4: {
                        chat::UserRequest userList;
                        userList.set_option(2);

                        userList.mutable_inforequest() -> set_type_request(true);

                        string serialized;
                        userList.SerializeToString(&serialized);

                        send(serverSocket, serialized.c_str(), serialized.length(), 0);

                        break;
                    }

                    case 5: {
                        string recipient;

                        printf("Enter user: ");
                        scanf("%s", buffer);
                        recipient = (string)buffer;

                        chat::UserRequest userInfo;
                        userInfo.set_option(2);

                        userInfo.mutable_inforequest() -> set_type_request(false);
                        userInfo.mutable_inforequest() -> set_user(recipient);

                        string serialized;
                        userInfo.SerializeToString(&serialized);

                        send(serverSocket, serialized.c_str(), serialized.length(), 0);

                        break;
                    }

                    case 6: {
                        printf("- In order to send a private message, enter '1' and then you must enter the recipient's email and the message you want to send\n");
                        printf("- In order to send a public message, enter '2' and then you must enter the message you want to send\n");
                        printf("- In order to change your status, enter '3' and then you must enter the number of the status you want to change to\n");
                        printf("- In order to get a list of users, enter '4'\n");
                        printf("- In order to get information about a user, enter '5' and then you must enter the user's email\n");
                        printf("- In order to exit, enter '7'\n\n");
                        break;
                    }

                    case 7: {
                        printf("Exiting... Please wait\n");
                        *running = false;
                        break;
                    }

                    default: {
                        printf("Invalid choice\n");
                        break;
                    }
                }
            }
            wait(NULL);
        }
        wait(NULL);
    }

    close(serverSocket);
    shutdown(serverSocket, SHUT_RDWR);
    google::protobuf::ShutdownProtobufLibrary();

    printf("Client is shutting down\n");
    munmap(&running, sizeof(bool));
    return 0;
}