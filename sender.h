#include <iostream>
#include <cstring>

#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <libgen.h>
#include <sys/stat.h>
#include <sys/sendfile.h>

class sender_t
{
    public:
    sender_t();
    ~sender_t();
    
    int transfer();
    const char* file;
    const char* server;
    
    private:
    short port;
    
    int start();
    int get_file();
    void notify( const char*, int );
};
