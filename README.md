### Protobuf compilation
protoc project.proto --cpp_out=./

### Client compilation
g++ -std=c++11 Client.cpp -o client -lprotobuf project.pb.cc

### Server compilation
g++ -std=c++11 Server.cpp -o server -lpthread -lprotobuf project.pb.cc

### Running server
./server <port>

### Running client
./client <ip> <port>