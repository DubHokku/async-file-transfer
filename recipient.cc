#include "recipient.h"

recipient_t::recipient_t()
{
    port = 127;
    timeout = 17;
    session_data = new session_t;
    transfer_count = ATOMIC_VAR_INIT( 0 );
    transfer_flag = false;
}

recipient_t::~recipient_t()
{
    delete session_data;
}

void recipient_t::receive()
{
    int result, max_d_num;
    server_socket = this->start();
    
    char data[4344]; // ~ mss x 3
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
            FD_SET( session->session_socket, &sock_d_set );
            max_d_num = session->session_socket > max_d_num?session->session_socket:max_d_num;
        }
        
        if(( select( max_d_num + 1, &sock_d_set, NULL, NULL, NULL )) < 0 )
        {
            if( errno == EINTR )
                continue;
            notify( "select()", errno );
        }
        
        for( session = sessions.begin(); session != sessions.end(); session++ )
        {
            memset( data, 0, sizeof( data ));
            result = recv( session->session_socket, data, sizeof( data ), 0 );
            if( result < 0 )
            {
                if(( errno != EINTR ) && ( errno != EAGAIN ) && ( errno != EWOULDBLOCK ))
                {
                    std::cout << "error on recv() " << strerror( errno ) << std::endl;
                    tmp_link = session;
                    session++;
                    
                    shutdown( tmp_link->session_socket, 2 );
                    close( tmp_link->file );
                    delete( tmp_link->file_name );
                    sessions.erase( tmp_link );
                    
                    std::atomic_fetch_sub( &transfer_count, 1 );
                    transfer_flag = false;
                    transfer.notify_all();
                    
                    continue;
                }
            }
            else
                if( result == 0 )
                {
                    std::cout << "End Of Data, session " << session->session_socket << " close" << std::endl;
                    tmp_link = session;
                    session++;
                    
                    shutdown( tmp_link->session_socket, 2 );
                    close( tmp_link->file );
                    delete( tmp_link->file_name );
                    sessions.erase( tmp_link );
                    
                    std::atomic_fetch_sub( &transfer_count, 1 );
                    transfer_flag = false;
                    transfer.notify_all();
                    
                    continue;
                }
                else
                {
                    transfer_flag = true;
                    if( session->name_size == 0 )
                    {                           
                        session->name_size = strlen( data );
                        session->file_name = new char[sizeof( directory ) + session->name_size];
                        
                        strcpy( session->file_name, directory );
                        strncat( session->file_name, data, session->name_size );
                        
                        std::cout << "new data " << data << std::endl;
                        session->file = open( session->file_name,  O_RDWR | O_CREAT, 00644 );
                        
                        if( send( session->session_socket, data, session->name_size, 0 ) < 0 )
                            notify( "send()", errno );
                        
                        continue;
                    }
                    else
                        write( session->file, data, result );
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
                    if( fcntl( result, F_SETFL, O_NONBLOCK ) < 0 )
                        notify( "fcntl()", errno );
                    
                    session_data->session_socket = result;
                    session_data->name_size = 0;
                    session_data->file_name = NULL;
                    sessions.push_back( *session_data );
                    
                    std::atomic_fetch_add( &transfer_count, 1 );
                    
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
    std::unique_lock<std::mutex> lck( lock );
   
    while( transfer_count )
    {
        if( transfer.wait_for( lck, std::chrono::seconds( timeout )) == std::cv_status::timeout )
        {
            if( transfer_flag )
                transfer_flag = false;
            else
                transfer_count = ATOMIC_VAR_INIT( 0 );
        }
    }
    
    for( session = sessions.begin(); session != sessions.end(); session++ )
    {
        std::cout << "close file " << session->file_name << std::endl;
        shutdown( session->session_socket, 2 );
        close( session->file );
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
