#include <iostream>       // For standard I/O operations
#include <thread>         // For threads
#include <algorithm>      // For std::count
#include <string>         // For std::string
#include <vector>         // For std::vector
#include <unordered_map>  // For std::unordered_map
#include <unordered_set>  // For std::unordered_set
#include <mutex>          // For std::mutex
#include <sys/socket.h>   // For socket programming
#include <arpa/inet.h>    // For inet_pton, inet_ntop
#include <fcntl.h>        // For file control options
#include <unistd.h>       // For close()
#include <errno.h>        // For errno error checking
#include <cstring>        // For strerror
#include <openssl/hmac.h> // For HMAC operations
#include <openssl/sha.h>  // For SHA hashing

#define TOKEN_EXPIRY_DURATION 36000     //: Token expiry duration in seconds (10 hour)
#define SECRET_KEY "chin_tapak_dum_dum" //: Secret key for HMAC operations
#define PIECE_SIZE 1024                 //: Piece size for file chunking (in bytes)

using namespace std;

class Users;  /// Forward declaration of Users class
class Group;  /// Forward declaration of Group class
class Groups; /// Forward declaration of Groups class

class Socket {
    private:
        std::string m_ip;      ///< IP address for the socket
        int m_port;            ///< Port number for the socket
        int m_socketFd{-1};    ///< File descriptor for the socket, initialized to -1 (invalid)

    public:
        /// Default constructor
        Socket() = default;

        /// Parameterized constructor to initialize IP address and port
        /// @param ip The IP address to bind the socket to
        /// @param port The port number to bind the socket to
        Socket(std::string ip, int port) 
            : m_ip(ip), m_port(port) 
        {}

        /// Create a socket and obtain a file descriptor
        void createSocket();

        /// Set socket options, such as reuse address & ports
        void setOptions();

        /// Bind the socket to the IP address and port
        void bindSocket();

        /// Prepare the socket to listen for incoming connections
        void listenSocket();

        /// Accept an incoming connection and return the client socket file descriptor
        /// @return The file descriptor for the accepted client socket
        int acceptSocket();

        /// Connect to a remote socket (typically a tracker) using the provided IP and port
        /// @param clientSocket The file descriptor for the client socket to use
        /// @param trackerIp The IP address of the tracker to connect to
        /// @param trackerPort The port number of the tracker to connect to
        void connectSocket(int clientSocket, std::string trackerIp, int trackerPort);

        /// Send data over the socket
        /// @param clientSocketFd The file descriptor for the client socket
        /// @param response The data to send
        void sendSocket(int& clientSocketFd, std::string& response);

        /// Receive data from the socket
        /// @param clientSocketFd The file descriptor for the client socket
        /// @return The received data as a string
        std::string recvSocket(int& clientSocketFd);

        /// Close the socket and release associated resources
        void closeSocket();
};


/// Represents a file with associated metadata and access control
/// Private Class
class File {
    friend class Users;   ///< Allows the Users class to access private members of File
    friend class Group;   ///< Allows the Group class to access private members of File
    friend class Groups;  ///< Allows the Groups class to access private members of File

    private:
        /// Parameterized constructor to initialize file metadata and access control
        /// @param fileName The name of the file
        /// @param SHA The SHA hash values associated with the file
        /// @param size The size of the file in bytes
        /// @param userName The set of usernames that have access to the file
        File(std::string fileName, std::vector<std::string> SHA, int size, std::unordered_set<std::string> userName)
            : m_fileName(fileName)
            , m_SHA(SHA)
            , m_size(size)
            , m_userNames(userName)
        {}

        std::string m_fileName;                         ///< The name of the file
        std::vector<std::string> m_SHA;                 ///< SHA hash values for file integrity verification
        int m_size;                                     ///< The size of the file in bytes
        std::unordered_set<std::string> m_userNames;    ///< Set of usernames with access to the file
    
    public:
        /// Default constructor
        File() = default;  
};


/// Represents a user with authentication credentials and group memberships
/// Private class
class User {
    friend class Users;  ///< Allows the Users class to access private members of User
    friend class Groups; ///< Allows the Groups class to access private members of User

    private:
        /// Parameterized constructor to initialize user credentials
        /// @param userName The username for the user
        /// @param password The password for the user
        User(std::string userName, std::string password)
            : m_userName(userName)
            , m_password(password)
        {}

        std::string m_userName;                     ///< The username for the user
        std::string m_password;                     ///< The password for the user (should be securely hashed in practice)
        std::unordered_set<std::string> m_groups;   ///< Set of group names that the user belongs to
    
    public:
        /// Default constructor
        User() = default;  
};

/// Represents a group with associated participants, pending join requests, and files
/// Private class
class Group {
    friend class Users;   ///< Allows the Users class to access private members of Group
    friend class Groups;  ///< Allows the Groups class to access private members of Group

    private:
        /// Parameterized constructor to initialize group metadata
        /// @param groupName The name of the group
        /// @param participants The list of participants in the group
        Group(std::string groupName, std::vector<std::string> participants)
            : m_groupName(groupName)
            , m_participants(participants)
        {}

        std::string m_groupName;                        ///< The name of the group
        std::vector<std::string> m_participants;        ///< List of participants in the group
        std::unordered_set<std::string> m_pendingJoins; ///< Set of usernames that have requested to join the group
        std::unordered_map<std::string, File> m_files;  ///< Map of files associated with the group, where the key is the file name
    
    public:
        /// Default constructor
        Group() = default; 
};

/// Utility class providing static methods for token management and argument processing
/// This class is intended to be used as a namespace for utility functions only.
/// It cannot be instantiated due to the deleted constructor.
class Utils {
    friend class Users;   ///< Allows the Users class to access private members of Utils
    friend class Groups;  ///< Allows the Groups class to access private members of Utils

    private:
        /// Deleted default constructor to prevent instantiation of Utils class
        Utils() = delete;

        /// Generate a token based on the provided payload
        /// @param payload The data to be encoded into the token
        /// @return The generated token as a string
        static std::string generateToken(std::string& payload);

        /// Validate the provided token
        /// @param token The token to be validated
        /// @return The validation result as a string
        static std::string validateToken(std::string& token);

    public:
        /// Process command-line arguments and return them as a pair
        /// @param argc The number of command-line arguments
        /// @param argv The array of command-line argument strings
        /// @return A pair containing the processed arguments (string) and an integer status code
        static std::pair<std::string, int> processArgs(int argc, char* argv[]);

        /// Tokenize a string into a vector of substrings based on a specified separator
        /// @param buffer The string to be tokenized
        /// @param separator The character used to separate tokens
        /// @return A vector of substrings extracted from the buffer
        static std::vector<std::string> tokenize(std::string buffer, char separator);
};


/// Singleton class responsible for managing user instances and their associated IP addresses
class Users {
    friend class Groups;  ///< Allows the Groups class to access private members of Users

    private:
        /// Default constructor (private to enforce singleton pattern)
        Users() = default;

        /// Destructor (default implementation)
        ~Users() = default;

        /// Copy constructor (deleted to prevent copying of the singleton instance)
        Users(const Users&) = delete;

        /// Assignment operator (deleted to prevent assignment of the singleton instance)
        Users& operator=(const Users&) = delete;

        static Users* instance;  ///< Pointer to the single instance of Users

        static std::mutex usersMutex;       ///< Mutex for synchronizing access to user data
        static std::mutex userToIpMutex;    ///< Mutex for synchronizing access to user-to-IP mapping

        static std::unordered_map<std::string, User> users;             ///< Map of usernames to User objects
        static std::unordered_map<std::string, std::string> userToIp;   ///< Map of usernames to IP addresses
    
    public:
        /// Get the singleton instance of the Users class
        /// @return Reference to the single instance of Users
        static Users& getInstance() {
            static Users instance;  ///< Guaranteed to be destroyed, instantiated on first use
            return instance;
        }

        /// Add a new user to the system
        /// @param userName The username of the new user
        /// @param password The password of the new user
        /// @return Status message indicating success or failure
        std::string addUser(std::string& userName, std::string& password);

        /// Log in a user and provide an authentication token
        /// @param userName The username of the user
        /// @param password The password of the user
        /// @param seederIpPort The IP and port of the seeder for the login process
        /// @return Authentication token or error message
        std::string loginUser(std::string& userName, std::string& password, std::string& seederIpPort);

        /// Log out a user and invalidate their authentication token
        /// @param authToken The authentication token of the user to log out
        /// @return Status message indicating success or failure
        std::string logoutUser(std::string& authToken);
};

/// Singleton class responsible for managing group operations and group memberships
class Groups {
    private:
        /// Default constructor (private to enforce singleton pattern)
        Groups() = default;

        /// Destructor (default implementation)
        ~Groups() = default;

        /// Copy constructor (deleted to prevent copying of the singleton instance)
        Groups(const Groups&) = delete;

        /// Assignment operator (deleted to prevent assignment of the singleton instance)
        Groups& operator=(const Groups&) = delete;

        static Groups* instance;                                ///< Pointer to the single instance of Groups
        static std::mutex groupsMutex;                          ///< Mutex for synchronizing access to group data
        static std::unordered_map<std::string, Group> groups;   ///< Map of group names to Group objects

    public:
        /// Get the singleton instance of the Groups class
        /// @return Reference to the single instance of Groups
        static Groups& getInstance() {
            static Groups instance;  // Guaranteed to be destroyed, instantiated on first use
            return instance;
        }

        /// Add a new group to the system
        /// @param groupName The name of the new group
        /// @param authToken The authentication token of the user performing the operation
        /// @return Status message indicating success or failure
        std::string addGroup(std::string& groupName, std::string& authToken);

        /// Join an existing group
        /// @param groupName The name of the group to join
        /// @param authToken The authentication token of the user attempting to join the group
        /// @return Status message indicating success or failure
        std::string joinGroup(std::string& groupName, std::string& authToken);

        /// List pending join requests for a group
        /// @param groupName The name of the group to list requests for
        /// @param authToken The authentication token of the user requesting the information
        /// @return List of pending join requests or error message
        std::string listRequests(std::string& groupName, std::string& authToken);

        /// List all groups available in the system
        /// @param authToken The authentication token of the user requesting the information
        /// @return List of groups or error message
        std::string listGroups(std::string& authToken);

        /// Accept a pending join request for a group
        /// @param groupName The name of the group for which to accept the request
        /// @param pendingUserName The username of the user whose request is being accepted
        /// @param authToken The authentication token of the user performing the operation
        /// @return Status message indicating success or failure
        std::string acceptRequest(std::string& groupName, std::string& pendingUserName, std::string& authToken);

        /// List all files associated with a group
        /// @param groupName The name of the group to list files for
        /// @param authToken The authentication token of the user requesting the information
        /// @return List of files or error message
        std::string listFiles(std::string& groupName, std::string& authToken);

        /// Upload a file to a group
        /// @param fileName The name of the file to upload
        /// @param groupName The name of the group to upload the file to
        /// @param fileSize The size of the file being uploaded
        /// @param SHAs The SHA hash values for the file
        /// @param authToken The authentication token of the user performing the upload
        /// @return Status message indicating success or failure
        std::string uploadFiles(std::string& fileName, std::string& groupName, std::string& fileSize, std::string& SHAs, std::string& authToken);

        /// Download a file from a group
        /// @param fileName The name of the file to download
        /// @param groupName The name of the group from which to download the file
        /// @param authToken The authentication token of the user performing the download
        /// @return Status message indicating success or failure
        std::string downloadFile(std::string& fileName, std::string& groupName, std::string& authToken);

        /// Stop sharing a file in a group
        /// @param groupName The name of the group to stop sharing the file in
        /// @param fileName The name of the file to stop sharing
        /// @param authToken The authentication token of the user performing the operation
        /// @return Status message indicating success or failure
        std::string stopShare(std::string& groupName, std::string& fileName, std::string& authToken);

        /// Leave a group
        /// @param groupName The name of the group to leave
        /// @param authToken The authentication token of the user leaving the group
        /// @return Status message indicating success or failure
        std::string leaveGroup(std::string& groupName, std::string& authToken);
};


extern Users& users;   ///< Reference to the singleton instance managing user operations
extern Groups& groups; ///< Reference to the singleton instance managing group operations

/// Handle the "quit" command from a tracker, which closes the socket and exits the program
/// @param socket The socket associated with the tracker
void handleTrackerQuit(Socket socket);

/// Handle incoming client requests by processing commands and responding accordingly
/// @param socket The socket associated with the client
/// @param clientSocketFd The file descriptor for the client's socket
void handleClientRequest(Socket socket, int clientSocketFd);

/// Execute a command received from a client
/// @param command The command to be executed
/// @return A response string indicating the result of the command execution
std::string executeCommand(std::string& command);
