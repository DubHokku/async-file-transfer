#include <iostream>
#include <list>
#include <iterator>
#include <cstring>
#include <string>
#include <csignal>

#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>

#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

class recipient_t
{
    public:
    recipient_t();
    ~recipient_t();
    
    void receive();
    void stop( int );
    
    private:
    int start();
    void notify( const char*, int );
    
    struct session_t
    {
        int file;
        int session_socket;
        short name_size;
        char* file_name;
    };
    
    short port, timeout;
    int server_socket;
    session_t *session_data;
    
    std::atomic < int > transfer_count;
    std::atomic < bool > transfer_flag;
    std::mutex lock;
    std::condition_variable transfer;
    
    std::list < session_t > sessions;
    std::list < session_t > :: iterator session;
    std::list < session_t > :: iterator tmp_link;
};
