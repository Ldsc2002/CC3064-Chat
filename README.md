### Protobuf compilation
protoc project.proto --cpp_out=./

### Client compilation
g++ Client.cpp -o client -lprotobuf project.pb.cc

### Server compilation
g++ Server.cpp -o server -lpthread -lprotobuf project.pb.cc