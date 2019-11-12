#include <iostream>
#include <csignal>
#include <atomic>
#include <thread>

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

static void mkdaemon()
{
    pid_t ps_ident = 0;
    ps_ident = fork();
    
    if( ps_ident < 0 )
        exit( EXIT_FAILURE );
    if( ps_ident > 0 )
        exit( EXIT_SUCCESS );
    if( setsid() < 0 )
        exit( EXIT_FAILURE );
    
    signal( SIGCHLD, SIG_IGN );
    ps_ident = fork();

    if( ps_ident < 0 )
        exit( EXIT_FAILURE );
    if( ps_ident > 0 )
        exit( EXIT_SUCCESS );

    // child_thread++;
    for( int fd = sysconf( _SC_OPEN_MAX ); fd > 0; fd-- )
        close( fd );

    stdin = fopen( "/dev/null", "r" );
    stdout = fopen( "/dev/null", "w+" );
    stderr = fopen( "/dev/null", "w+" );
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
    {    
        mkdaemon();
        std::thread receive( &recipient_t::receive, &server );
        receive.join();
    }
    else
        std::cout << "type " << av[0] << " ( server | <file> <server address> )" << std::endl;
    
    return 0;
}
