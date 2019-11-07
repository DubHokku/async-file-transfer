#include "sender.h"

sender_t::sender_t()
{
    port = 127;
}

sender_t::~sender_t(){}

int sender_t::transfer()
{
    int data = get_file();
    int socket = start();
    
    char* file_name = basename(( char* )file );
    int name_lenght = strlen( file_name );
 
    std::cout << "transfer " << file_name << " to " << server << std::endl;
    
    struct stat file_stat;
    if( stat( file, &file_stat ) < 0 )
        notify( "stat()", errno );

    if( send( socket, file_name, name_lenght, 0 ) < 0 )
        notify( "send()", errno );
    if( recv( socket, file_name, name_lenght, 0 ) < 0 )
        notify( "recv()", errno );

    ssize_t ret = sendfile( socket, data, NULL, file_stat.st_size );
    std::cout << "sendfile() ret. " << ret << std::endl;
    
    close( data );
    shutdown( socket, 1 );
    
    return 0;
}

int sender_t::start()
{
    int descriptor = socket( AF_INET, SOCK_STREAM, 0 );
    if( descriptor < 0 )
        notify( "soket()", errno );

    struct sockaddr_in server_sockaddr;
    server_sockaddr.sin_addr.s_addr = inet_addr( server );
    server_sockaddr.sin_family = AF_INET;
    server_sockaddr.sin_port = htons( port );

    if( connect( descriptor, ( struct sockaddr* )&server_sockaddr, sizeof( server_sockaddr )) < 0 )
        notify( "connect()", errno );

    return descriptor;
}

int sender_t::get_file()
{
    int descriptor = open( file, O_RDONLY );
    if( descriptor < 0 )
        notify( "open()", errno );
    
    return descriptor;
}

void sender_t::notify( const char* func, int code )
{
    std::cout << func << ": " << strerror( code ) << std::endl;
    exit( 0 );
}
