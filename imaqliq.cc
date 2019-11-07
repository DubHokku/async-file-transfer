#include <iostream>
#include <csignal>
#include <atomic>

#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#ifndef SERVER_H
#define SERVER_H
#include "recipient.h"
#endif // SERVER_H

#ifndef CLIENT_H
#define CLIENT_H
#include "sender.h"
#endif // CLIENT_H

recipient_t server;

void interr_handler( int signo )
{
    // std::atomic_signal_fence( std::memory_order_acq_rel );
    std::atomic_signal_fence( std::memory_order_seq_cst );
    server.stop( signo );
}

void signal_set()
{
    struct sigaction sa;
    sigset_t newset;
    sigemptyset( &newset );
    sigaddset( &newset, SIGHUP );
    sigaddset( &newset, SIGTERM );
    sa.sa_handler = interr_handler;
    sigaction( SIGTERM, &sa, 0 );
    sigaction( SIGHUP, &sa, 0 );
    std::cout << "proc.ident: " << getpid() << std::endl;
}

int main( int ac, char **av )
{
    signal_set();

    if( ac < 2 )
    {
        std::cout << "type " << av[0] << " ( server | <file> <server address> )" << std::endl;
        return 0;
    }
    
    if( ac > 2 )
    {
        sender_t client;
        client.file = av[1];
        client.server = av[2];
        
        client.transfer();
        
        return 0;
    }
    
    if( strcmp( av[1], "server" ) == 0 )
        server.receive();
    else
        std::cout << "type " << av[0] << " ( server | <file> <server address> )" << std::endl;
    
    return 0;
}
