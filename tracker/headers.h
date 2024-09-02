#include <iostream>       // For standard I/O operations
#include <string>         // For std::string
#include <vector>         // For std::vector
#include <unordered_map>  // For std::unordered_map
#include <unordered_set>  // For std::unordered_set
#include <mutex>          // For std::mutex
#include <sys/socket.h>   // For socket programming
#include <arpa/inet.h>    // For inet_pton, inet_ntop
#include <fcntl.h>        // For file control options (if needed)
#include <errno.h>        // For errno error checking (if needed)
#include <openssl/hmac.h> // For HMAC operations
#include <openssl/sha.h>  // For SHA hashing

#define TOKEN_EXPIRY_DURATION 36000     //: Token expiry duration in seconds (10 hour)
#define SECRET_KEY "chin_tapak_dum_dum" //: Secret key for HMAC operations
#define PIECE_SIZE 1024                 //: Piece size for file chunking (in bytes)

using namespace std;

//: Class representing a single file
//: Private Class
class File {
    friend class Users;   //: Granting access to Users class
    friend class Groups;  //: Granting access to Groups class

    private:
        File() = default;  //: Default constructor for internal use only
        File(string fileName, vector<string> SHA, int size, string userName);

        string fileName;                    //: Name of the file
        vector<string> SHA;                 //: Vector containing SHA hashes; index 0 for entire file, rest for pieces
        int size;                           //: Size of the file in bytes
        unordered_set<string> userNames;    //: Set of userNames having access to this file
};

//: Class representing a single user
//: Private Class
class User {
    friend class Users;   //: Granting access to Users class
    friend class Groups;  //: Granting access to Groups class

    private:
        User() = default;  //: Default constructor for internal use only
        User(string userName, string password);

        string userName;                    //: Username of the user
        string password;                    //: Password of the user
        unordered_set<string> groups;       //: Set of group names the user belongs to
};

//: Class representing a single group
//: Private class
class Group {
    friend class Users;   //: Granting access to Users class
    friend class Groups;  //: Granting access to Groups class

    private:
        Group() = default;  //: Default constructor for internal use only
        Group(string groupName, string admin);

        string groupName;                       //: Name of the group
        vector<string> participants;            //: List of participants (userNames) in the group, particiants[0] is an admin
        unordered_set<string> pendingJoins;     //: Set of pending join requests (userNames)
        unordered_map<string, File> files;      //: Map of files shared within the group, keyed by filename
};

//: Utility class providing helper functions for args preprocessing, tokenization and authToken generation/validation
class Utils {
    friend class Users;   //: Granting access to Users class
    friend class Groups;  //: Granting access to Groups class

    private:
        //: Generates a authToken based on the provided payload
        string generateToken(string& payload);

        //: Validates a authToken and returns userName
        string validateToken(string& token);

    public:
        //: Processes command-line arguments and returns a pair of IP(string) and Port(int)
        pair<string, int> processArgs(int& argc, char* argv[]);

        //: Splits a string buffer into a vector of tokens based on the separator
        vector<string> tokenize(string& buffer, char& separator);
};

//: Singleton class managing users in the system
class Users {
    friend class Groups;    //: Granting access to Groups class

    private:
        Users() = default;      //: Private constructor to prevent external instantiation
        ~Users() = default;     //: Private destructor

        //: Delete copy constructor and assignment operator to enforce singleton pattern
        Users(const Users&) = delete;
        Users& operator=(const Users&) = delete;

        //: Static instance pointer for singleton pattern
        static Users* instance;

        //: Mutex for thread-safe access to user data
        mutex usersMutex;
        mutex userToIpMutex;

        //: Map of Users keyed by userName
        unordered_map<string, User> users;
        //: Map of user IP addresses keyed by userName
        unordered_map<string, string> userToIp;

    public:
        //: Static method to get the singleton instance of Users class
        static Users& getInstance() {
            static Users instance;
            return instance;
        }

        //: Adds a new user to the system
        string addUser(string& userName, string& password);

        //: Login a user and returns a session token
        string loginUser(string& userName, string& password, string& seederIpPort);

        //: Logout a user and invalidates the session token
        string logoutUser(string& authtoken);
};

//: Singleton class managing groups in the system
class Groups {
    private:
        Groups() = default;  //: Private constructor to prevent external instantiation
        ~Groups() = default; //: Private destructor

        //: Delete copy constructor and assignment operator to enforce singleton pattern
        Groups(const Groups&) = delete;
        Groups& operator=(const Groups&) = delete;

        //: Static instance pointer for singleton pattern
        static Groups* instance;

        //: Mutex for thread-safe access to group data
        mutex groupsMutex;

        //: Map of groups keyed by group name
        unordered_map<string, Group> groups;

    public:
        //: Static method to get the singleton instance of Groups class
        static Groups& getInstance() {
            static Groups instance;
            return instance;
        }

        //: Adds a new group to the system
        string addGroup(string& groupName, string& authToken);

        //: Allows a user to join an existing group
        string joinGroup(string& groupName, string& authToken);

        //: Lists pending join requests for a group
        string listRequests(string& groupName, string& authToken);

        //: Lists all available groups in the system
        string listGroups(string& authToken);

        //: Accepts a pending join request for a group
        string acceptRequest(string& groupName, string& pendingUserName, string& authToken);

        //: Lists all files shared in a group
        string listFiles(string& groupName, string& authToken);

        //: Uploads a file to a group
        string uploadFiles(string& fileName, string& groupName, string& fileSize, string& SHAs, string& authToken);

        //: Downloads a file from a group
        string downloadFile(string& fileName, string& groupName, string& authToken);

        //: Stops sharing a file in a group
        string stopShare(string& groupName, string& fileName, string& authToken);

        //: Allows a user to leave a group
        string leaveGroup(string& groupName, string& authToken);
};

