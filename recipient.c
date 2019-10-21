#include "recipient.h"

recipient_t::recipient_t()
{
    port = 127;
}

recipient_t::~recipient_t(){}

void recipient_t::receive()
{
    int result, max_d_num;
    server_socket = start();
    session_t *session_data;
    
    char data[4344]; // segment size x 3 ( L2: 14 byte, L3: 20 byte, L4: 20 + 32 byte => size 1448 )
    char directory[] = "upload/";
    
    if( mkdir( directory, 0755 ) < 0 )
        if( errno != EEXIST )
            notify( "mkdir()", errno );
    
    while( true )
    {
        fd_set sock_d_set;
        FD_ZERO( &sock_d_set );
        FD_SET( server_socket, &sock_d_set );
        max_d_num = server_socket;
        
        for( session = sessions.begin(); session != sessions.end(); session++ )
        {
            FD_SET(( *session )->session_socket, &sock_d_set );
            max_d_num = ( *session )->session_socket > max_d_num?( *session )->session_socket:max_d_num;
        }
        
        if(( select( max_d_num + 1, &sock_d_set, NULL, NULL, NULL )) < 0 )
        {
            if( errno == EINTR )
                continue;
            notify( "select()", errno );
        }
        
        for( session = sessions.begin(); session != sessions.end(); session++ )
        {
            result = recv(( *session )->session_socket, data, sizeof( data ), 0 );
            if( result < 0 )
            {
                if(( errno != EINTR ) && ( errno != EAGAIN ) && ( errno != EWOULDBLOCK ))
                {
                    std::cout << "error on recv() " << strerror( errno ) << std::endl;
                    tmp_link = session;
                    session++;
                    
                    shutdown(( *tmp_link )->session_socket, 2 );
                    close(( *tmp_link )->file );
                    delete(( *tmp_link )->file_name );
                    delete(( *tmp_link ));
                    sessions.erase( tmp_link );
                    continue;
                }
            }
            else
                if( result == 0 )
                {
                    std::cout << "End Of Data, session " << ( *session )->session_socket << " close" << std::endl;
                    tmp_link = session;
                    session++;
                    
                    shutdown(( *tmp_link )->session_socket, 2 );
                    close(( *tmp_link )->file );
                    delete(( *tmp_link )->file_name );
                    delete(( *tmp_link ));
                    sessions.erase( tmp_link );
                    continue;
                }
                else
                {
                    if(( *session )->name_size == 0 )
                    {
                        memcpy( &( *session )->name_size, ( void* )data, sizeof( short ));
                        std::cout << "received 'name size' " << ( *session )->name_size << " byte" << std::endl;
                        continue;
                    }
                    if(( *session )->file_name == NULL )
                    {
                        ( *session )->file_name = new char[sizeof( directory ) + ( *session )->name_size];
                        
                        memset(( *session )->file_name, 0, sizeof( directory ) + ( *session )->name_size );
                        strcpy(( *session )->file_name, directory );
                        strncat(( *session )->file_name, data, ( *session )->name_size );
                        ( *session )->file_name[sizeof( directory) + ( *session )->name_size] = 0;
                        
                        std::cout << "new path " << ( *session )->file_name << std::endl;
                        
                        ( *session )->file = open(( *session )->file_name,  O_RDWR | O_CREAT, 00644 );
                    }
                    else
                    {
                        // std::cout << ( *session )->session_socket << " receive " << result << " byte" << std::endl;
                        write(( *session )->file, data, result );
                    }
                }
        }
        
        if( FD_ISSET( server_socket, &sock_d_set ))
        {
            while( true )
            {
                result = accept( server_socket, ( struct sockaddr* )NULL, NULL );
                if( result < 0 )
                {   
                    if( errno == EAGAIN || errno == EWOULDBLOCK )
                        break;
                    notify( "accept()", errno );
                }
                else
                {
                    session_data = new session_t;
                    
                    if( session_data == NULL )
                        notify( "new()", 0 );
                    if( fcntl( result, F_SETFL, O_NONBLOCK ) < 0 )
                        notify( "fcntl()", errno );
                    
                    session_data->session_socket = result;
                    session_data->name_size = 0;
                    session_data->file_name = NULL;
                    
                    sessions.push_back( session_data );
                    std::cout << "new connection, socket: " << session_data->session_socket << std::endl;
                }
            }
        }
    }
}

void recipient_t::stop( int code )
{
    std::cout << "server.stop() interrupt: " << code << std::endl;
    std::cout << "server have " << sessions.size() << " connections and opened files" << std::endl;

    for( session = sessions.begin(); session != sessions.end(); session++ )
    {
        std::cout << "close file " << ( *session )->file_name << std::endl;
        shutdown(( *session )->session_socket, 2 );
        close(( *session )->file );
    }

    shutdown( server_socket, 1 );
    std::cout << "server stopped" << std::endl;

    exit( 0 );        
}

int recipient_t::start()
{
    int listen_socket, enable = 1;
    struct sockaddr_in listen_sockaddr;

    if(( listen_socket = socket( PF_INET, SOCK_STREAM, 0 )) < 0 )
        notify( "socket()", errno );
    
    setsockopt( listen_socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof( int ));

    memset( &listen_sockaddr, 0, sizeof( listen_sockaddr ));
    listen_sockaddr.sin_family = PF_INET;
    listen_sockaddr.sin_port = htons( port );
    listen_sockaddr.sin_addr.s_addr = INADDR_ANY;

    if( bind( listen_socket, ( struct sockaddr* )&listen_sockaddr, sizeof( listen_sockaddr )) < 0 )
        notify( "bind()", errno );
    if( listen( listen_socket, 5 ) < 0 )
        notify( "listen()", errno );
    if( fcntl( listen_socket, F_SETFL, O_NONBLOCK ) < 0 )
        notify( "fcntl()", errno );

    return listen_socket;
}

void recipient_t::notify( const char *func, int code )
{
    std::cout << func << ": " << strerror( code ) << " code " << code << std::endl;
    exit( 0 );
}
