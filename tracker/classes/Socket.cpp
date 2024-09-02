#include "../headers.h"

//: Declaration
// class Socket{
//     private:
//         string m_ip;
//         int m_port;
//         int m_socketFd {-1};
    
//     public:
//         Socket() = default;
//         Socket(string ip, int port) : m_ip(ip), m_port(port) {};

//         void createSocket();
//         void setOptions();
//         void bindSocket();
//         void listenSocket();
//         int acceptSocket();
//         void connectSocket(int clientSocket, string trackerIp, int trackerPort);
//         void sendSocket(int& clientSocketFd, stirng& response);
//         string recvSocket(int& clientSocketFd);
//         void closeSocket();
// };

void Socket::createSocket(){
    //: Much like everything in UNIX sockets are treated as file
    //: To create a socket, we need <sys/socket.h> header file
    int domainAddressFormat = AF_INET; //: Address Family (IPv4)
    int type = SOCK_STREAM; //: TCP
    int protocol = 0; //: TCP/IP
    
    //: socket() return file descriptor for that created socket
    m_socketFd = socket(domainAddressFormat, type, protocol);
    if(m_socketFd < 0){
        throw "Error at socket creation!!\nError: " + to_string(errno);
    }

}

void Socket::setOptions(){
    int opt = 1;
    if (setsockopt(m_socketFd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))){
        closeSocket();
        throw "Error at setOptions!!\nError: " + to_string(errno);
    }
}

void Socket::bindSocket(){
    //: Network Protocol uses Big-Endian Format for communication
    //: But different systems can have different fotmat specifications
    //: To get Big-Endian Format at the end, not depending on which particular Format you system is using
    //: We have some functions in <arpa/inet.h> library
    //: uint_32t htonl(uint32_t hostint32) // Translate 4 Byte int to network format (host to network long)
    //: uint_16t htons(uint16_t hostint16) // Translate 2 Byte int to network format (host to network short)
    //: uint_32t ntohl(uint32_t hostint32) // Translate 4 Byte int to host format (host to network long)
    //: uint_16t ntohs(uint16_t hostint16) // Translate 2 Byte int to host format (host to network short)

    //: An Internet Address (Socket Address) is represented by following structure
    //: struct sockaddr_in {
    //:     sa_family_t sin_family;
    //:     in_port_t sin_port;
    //:     struct in_addr sin_addr;
    //: }

    struct sockaddr_in trackerAddr;
    trackerAddr.sin_family = AF_INET; //: Address Family (IPv4)
    trackerAddr.sin_port = htons(m_port); //: Port
    trackerAddr.sin_addr.s_addr = htonl(INADDR_ANY); //: IPv4 Address of current computer 
    //: In above line we can set IP address also using inet_pton() function

    //: Binding socket with specific m_port
    //: If not binded it can associate the socket to any m_port
    if (bind(m_socketFd, (struct sockaddr*)&trackerAddr, sizeof(trackerAddr)) < 0) {
        closeSocket();
        throw "Error at bindSocket!!\nError: " + to_string(errno);
    }

}

void Socket::listenSocket(){
    //: listen returns fd
    //: We listen on a socket that has been bound with bind
    //: At a time we are allowing 100 connections waiting to be accepted in queue
    //: If queue is full the server will reject additional requests
    if (listen(m_socketFd, 5000) < 0) {
        closeSocket();
        throw "Error at listenSocket!!\nError: " + to_string(errno);
    }
    cout << "Listen Successfull\n" << flush;
}

int Socket::acceptSocket(){
    struct sockaddr_in clientAddr;
    socklen_t clientAddrSize = sizeof(clientAddr);
        
    //: It accepts incoming connection requests from queue
    int clientSocket = accept(m_socketFd, (struct sockaddr*)&clientAddr, &clientAddrSize);
    if (clientSocket < 0) {
        close(clientSocket);
        throw "Error at acceptSocket!!\nError:" + to_string(errno);
    }
    return clientSocket;   
}

void Socket::connectSocket(int clientSocket, string trackerIp, int trackerPort){
    //: An Internet Address (Socket Address) is represented by sockaddr_in structure
    //: Here we are prepareing Tracker's Internet Address
    struct sockaddr_in trackerAddr;
    trackerAddr.sin_family = AF_INET;
    trackerAddr.sin_port = htons(trackerPort);  

    //: Here we have IP address of tracker, If we have domain-name then first we need to convert it to IP
    //: Using getAddrInfo(string domainName, string Port/Protocol, struct addrinfo options, struct addrinfo *result)
    //: then we can use *result to get IP from struct addrinfo
    //: Here we have IP in string format so we can not use htonl
    //: Instead we use inet_pton(), converting it into uint_32t
    if (inet_pton(AF_INET, trackerIp.c_str(), &trackerAddr.sin_addr) <= 0) {
        throw "ClientError: Error converting IP address of Tracker!! Error:" + to_string(errno);
        closeSocket();
    }

    //: Connecting to tracker
    if (connect(clientSocket, (struct sockaddr*)&trackerAddr, sizeof(trackerAddr)) < 0) {
        throw "ClientError: Error connecting to Tracker" + to_string(errno);
        closeSocket();
    }
}

void Socket::closeSocket(){
    close(m_socketFd);
}

void Socket::sendSocket(int& clientSocketFd, string& response){
    string messageLength = to_string(response.size());
    response = messageLength + " " + response;

    if(send(clientSocketFd, response.c_str(), response.size(), 0) < 0){
        close(clientSocketFd);
        throw ("Error: sending message to client at fd " + to_string(clientSocketFd) + " " + string(strerror(errno))  + "\n");
    }
}

string Socket::recvSocket(int& clientSocketFd){
    int totalDataLength = -1;
    string receivedData = "";

    while(true){
        char buffer[524288];
        int bytesRead = recv(clientSocketFd, buffer, sizeof(buffer), 0);
        if (bytesRead <= 0) {
            close(clientSocketFd);
            throw string("Connection Closed: Client-socket at fd " + to_string(clientSocketFd) + " closed the connection\n");
        }

        receivedData += string(buffer, bytesRead);

        
        if(totalDataLength == -1) {
            vector<string> temp = Utils::tokenize(receivedData, ' ');
            totalDataLength = stoi(temp[0]) - (bytesRead - (temp[0].size() + 1));
            receivedData = receivedData.substr(temp[0].size() + 1);
        }
        else {
            totalDataLength -= bytesRead;
        }

        if(totalDataLength == 0) {
            break;
        }
    }

    return receivedData;
}

