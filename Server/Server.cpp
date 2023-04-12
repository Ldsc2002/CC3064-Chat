#include <iostream>
#include <unistd.h>
#include <pthread.h>
#include <netinet/in.h>
#include <cstring>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <fcntl.h>
#include "project.pb.h"

#define BUFFERSIZE 2048

using std::string;

struct Client {
    string username;
    string ip;
    int socket;
    int status;
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

bool checkIfUserExists(string ip, string email) {
    //for (int i = 0; i < 100; i++) {
    //   if (clients[i].ip == ip || clients[i].username == email) {
    //       return true;
    //   }
    //}

    return false;
}

void* clientHandler(void* arg) {
    pthread_t thisThread = pthread_self();
    int clientSlot = -1;
    int clientSocket = *(int *) arg;

    printf("Thread %lu is handling client\n", thisThread);

    char buffer[BUFFERSIZE] = {0};
    int readResult;

    bool* reading = (bool*)mmap(NULL, sizeof(bool), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    bool* noHeartbeat = (bool*)mmap(NULL, sizeof(bool), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    int flags = fcntl(clientSocket, F_GETFL, 0);
    fcntl(clientSocket, F_SETFL, flags | O_NONBLOCK);

    while (true) {
        memset(buffer, 0, BUFFERSIZE);

        *noHeartbeat = false;
        *reading = true;
        int pid = fork();
        
        if (pid == 0) {
            int timeInactive = 0;

            while (*reading) {
                sleep(1);                
                timeInactive += 1;

                if (timeInactive % 10 == 0) {
                    printf("Thread %lu: No heartbeat received - %d seconds\n", thisThread, timeInactive);
                }

                if (timeInactive >= 60) {
                    close(clientSocket);
                    *noHeartbeat = true;
                    break;
                }
            }

            return 0;

        } else if (pid > 0) {
            readResult = -1;

            while(readResult == -1 && !*noHeartbeat) {
                readResult = read(clientSocket, buffer, BUFFERSIZE);
            }

            *reading = false;
            wait(NULL);
        } 

        if (*noHeartbeat) {
            printf("Thread %lu: Client disconnected due to inactivity\n", thisThread);

            if (clientSlot != -1) {
                clients[clientSlot].username = "";
                clients[clientSlot].ip = "";
                clients[clientSlot].socket = 0;
                clients[clientSlot].status = 0;
            }
            
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
                clients[clientSlot].status = 0;
            }

            break;

        } else {
            chat::UserRequest newRequest;
            newRequest.ParseFromArray(buffer, BUFFERSIZE);

            printf("Received UserRequest:\n%s\n", newRequest.DebugString().c_str());


            if (newRequest.option() == 1) {
                // User registration
                printf("Thread %lu: User %s wants to register with IP %s\n", thisThread, newRequest.mutable_newuser() -> username().c_str(), newRequest.mutable_newuser() -> ip().c_str());

                chat::ServerResponse newResponse;
                newResponse.set_option(1);
                newResponse.set_code(400);
                newResponse.set_servermessage("Error registering user");

                if (checkIfUserExists(newRequest.mutable_newuser() -> ip(), newRequest.mutable_newuser() -> username())) {
                    printf("Thread %lu: User with IP %s or email %s already exists\n", thisThread, newRequest.mutable_newuser() -> ip().c_str(), newRequest.mutable_newuser() -> username().c_str());
                    newResponse.set_servermessage("User with this IP or Email already exists");

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
                    clients[clientSlot].status = 1;

                    newResponse.set_code(200);
                    newResponse.set_servermessage("User registered");

                    printf("Thread %lu: User %s registered with IP %s\n", thisThread, clients[clientSlot].username.c_str(), clients[clientSlot].ip.c_str());
                }

                string responseString;
                newResponse.SerializeToString(&responseString);

                send(clientSocket, responseString.c_str(), responseString.length(), 0);

            } else if (newRequest.option() == 2) {
                // User information request

                bool type_request = newRequest.mutable_inforequest() -> type_request(); 
                printf("%d", type_request);

                if (type_request == true) {
                    // All users
                    printf("Thread %lu: User %s wants to get all users\n", thisThread, clients[clientSlot].username.c_str());

                    chat::ServerResponse newResponse;
                    newResponse.set_option(2);
                    newResponse.set_code(200);
                    newResponse.set_servermessage("All users");

                    for (int i = 0; i < 100; i++) {
                        if (clients[i].username != "") {
                            chat::UserInfo* newUser = newResponse.mutable_connectedusers() -> add_connectedusers();
                            newUser -> set_username(clients[i].username);
                            newUser -> set_ip(clients[i].ip);
                            newUser -> set_status(clients[i].status);
                        }
                    }

                    string responseString;
                    newResponse.SerializeToString(&responseString);

                    send(clientSocket, responseString.c_str(), responseString.length(), 0);

                } else if (type_request == false) {
                    // Single user
                    std::string userSearch = newRequest.inforequest().user();

                    printf("Thread %lu: User %s wants to get user %s\n", thisThread, clients[clientSlot].username.c_str(), userSearch.c_str());

                    chat::ServerResponse newResponse;
                    newResponse.set_option(2);
                    newResponse.set_code(400);
                    newResponse.set_servermessage("User not found");

                    for (int i = 0; i < 100; i++) {
                        if (strcmp(clients[i].username.c_str(), userSearch.c_str()) == 0) {
                            printf("Thread %lu: User %s found\n", thisThread, clients[i].username.c_str());

                            chat::UserInfo* newUser = newResponse.mutable_connectedusers() -> add_connectedusers();
                            newUser -> set_username(clients[i].username);
                            newUser -> set_ip(clients[i].ip);
                            newUser -> set_status(clients[i].status);
                            
                            newResponse.set_code(200);
                            newResponse.set_servermessage("User found");
                            break;
                        }
                    }

                    string responseString;
                    newResponse.SerializeToString(&responseString);

                    send(clientSocket, responseString.c_str(), responseString.length(), 0);
                }
            } else if (newRequest.option() == 3) {
                bool success = false;
                // Status change
                for (int i = 0; i < 100; i++) {
                    if (clients[i].username == newRequest.mutable_status() -> username()) {
                        clients[i].status = newRequest.mutable_status() -> newstatus();
                        success = true;
                        break;
                    }
                }
                
                chat::ServerResponse newResponse;
                newResponse.set_option(3);

                if (success) {
                    printf("Thread %lu: User %s changed status to %d\n", thisThread, newRequest.mutable_status() -> username().c_str(), newRequest.mutable_status() -> newstatus());

                    newResponse.set_code(200);
                    newResponse.set_servermessage("Status changed");

                } else {
                    printf("Thread %lu: User %s not found\n", thisThread, newRequest.mutable_status() -> username().c_str());

                    newResponse.set_code(400);
                    newResponse.set_servermessage("User not found");
                }

                string responseString;
                newResponse.SerializeToString(&responseString);

                send(clientSocket, responseString.c_str(), responseString.length(), 0);

            } else if (newRequest.option() == 4) {
                // New message
                string newMsg = newRequest.mutable_message() -> message();
                string sender = newRequest.mutable_message() -> sender();
                string recipient;

                if (newRequest.mutable_message() -> message_type() == true) {
                    recipient = "all";
                } else {
                    recipient = newRequest.mutable_message() -> recipient();
                }


                bool online = false;
                int recipientSlot = -1;
                for (int i = 0; i < 100; i++) {
                    if (strcmp(clients[i].username.c_str(), recipient.c_str()) == 0) {
                        online = true;
                        recipientSlot = i;
                        break;
                    }
                }

                if (online || strcpm(recipient.c_str(), "all") == 0) {
                    printf("Thread %lu: New message from %s to %s: %s\n", thisThread, sender.c_str(), recipient.c_str(), newMsg.c_str());

                    chat::ServerResponse newResponse;
                    newResponse.set_option(4);
                    newResponse.set_code(200);
                    newResponse.set_servermessage("Message sent");

                    string responseString;
                    newResponse.SerializeToString(&responseString);

                    send(clientSocket, responseString.c_str(), responseString.length(), 0);

                    //clear buffer and send message to recipient
                    memset(buffer, 0, sizeof(buffer));

                    chat::ServerResponse sentMessage;
                    sentMessage.set_option(4);
                    sentMessage.set_code(200);
                    sentMessage.set_servermessage("New message");

                    chat::newMessage* finalMessage = sentMessage.mutable_message();
                    finalMessage -> set_sender(sender.c_str());
                    finalMessage -> set_message(newMsg.c_str());

                    string sentMsg;

                    if (recipient == "all") {
                        finalMessage -> set_message_type(true);
                        
                        sentMessage.SerializeToString(&sentMsg);

                        for (int i = 0; i < 100; i++) {
                            if (clients[i].username != sender && clients[i].status != 0) {
                                send(clients[i].socket, sentMsg.c_str(), sentMsg.size(), 0);
                            }
                        }
                    } else {
                        finalMessage -> set_message_type(false);
                        finalMessage -> set_recipient(recipient.c_str());

                        sentMessage.SerializeToString(&sentMsg);

                        for (int i = 0; i < 100; i++) {
                            if (strcmp(clients[i].username.c_str(), recipient.c_str()) == 0) {
                                send(clients[i].socket, sentMsg.c_str(), sentMsg.size(), 0);
                                break;
                            }
                        }
                    }

                } else {
                    printf("Thread %lu: User %s is offline\n", thisThread, recipient.c_str());

                    chat::ServerResponse newResponse;
                    newResponse.set_option(4);
                    newResponse.set_code(400);
                    newResponse.set_servermessage("User is offline");

                    string responseString;
                    newResponse.SerializeToString(&responseString);

                    send(clientSocket, responseString.c_str(), responseString.length(), 0);
                }

            } else if (newRequest.option() == 5) {
                // Heartbeat
                printf("Thread %lu: Heartbeat received\n", thisThread);

                chat::ServerResponse newResponse;
                newResponse.set_option(5);
                newResponse.set_code(200);
                newResponse.set_servermessage("Heartbeat received");

                string responseString;
                newResponse.SerializeToString(&responseString);

                send(clientSocket, responseString.c_str(), responseString.length(), 0);

            } else {
                // Unknown request
                printf("Thread %lu: Unknown request\n", thisThread);
            }
        }
    }

    munmap(&reading, sizeof(bool));
    munmap(&noHeartbeat, sizeof(bool));
    
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