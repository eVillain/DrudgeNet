#include "ConnectionTests.hpp"
#include "Connection.h"
#include <cassert>
#include <string>
#include <stdio.h>

using namespace Net;

#ifdef DEBUG
#define check assert
#else
#define check(n) if ( !(n) ) { printf( "check failed\n" ); exit(1); }
#endif

void test_connection_join()
{
    printf( "-----------------------------------------------------\n" );
    printf( "test connection join\n" );
    printf( "-----------------------------------------------------\n" );
    
    const int ServerPort = 30000;
    const int ClientPort = 30001;
    const int ProtocolId = 0x11112222;
    const float DeltaTime = 0.001f;
    const float TimeOut = 0.1f;
    
    Connection client( ProtocolId, TimeOut );
    Connection server( ProtocolId, TimeOut );
    
    check( client.Start( ClientPort ) );
    check( server.Start( ServerPort ) );
    
    client.Connect( Address(127,0,0,1,ServerPort ) );
    server.Listen();
    
    while ( true )
    {
        if ( client.IsConnected() && server.IsConnected() )
            break;
        
        if ( !client.IsConnecting() && client.ConnectFailed() )
            break;
        
        unsigned char client_packet[] = "client to server";
        client.SendPacket( client_packet, sizeof( client_packet ) );
        
        unsigned char server_packet[] = "server to client";
        server.SendPacket( server_packet, sizeof( server_packet ) );
        
        while ( true )
        {
            unsigned char packet[256];
            int bytes_read = client.ReceivePacket( packet, sizeof(packet) );
            if ( bytes_read == 0 )
                break;
        }
        
        while ( true )
        {
            unsigned char packet[256];
            int bytes_read = server.ReceivePacket( packet, sizeof(packet) );
            if ( bytes_read == 0 )
                break;
        }
        
        client.Update( DeltaTime );
        server.Update( DeltaTime );
    }
    
    check( client.IsConnected() );
    check( server.IsConnected() );
}

void test_connection_join_timeout()
{
    printf( "-----------------------------------------------------\n" );
    printf( "test connection join timeout\n" );
    printf( "-----------------------------------------------------\n" );
    
    const int ServerPort = 30000;
    const int ClientPort = 30001;
    const int ProtocolId = 0x11112222;
    const float DeltaTime = 0.001f;
    const float TimeOut = 0.1f;
    
    Connection client( ProtocolId, TimeOut );
    
    check( client.Start( ClientPort ) );
    
    client.Connect( Address(127,0,0,1,ServerPort ) );
    
    while ( true )
    {
        if ( !client.IsConnecting() )
            break;
        
        unsigned char client_packet[] = "client to server";
        client.SendPacket( client_packet, sizeof( client_packet ) );
        
        while ( true )
        {
            unsigned char packet[256];
            int bytes_read = client.ReceivePacket( packet, sizeof(packet) );
            if ( bytes_read == 0 )
                break;
        }
        
        client.Update( DeltaTime );
    }
    
    check( !client.IsConnected() );
    check( client.ConnectFailed() );
}

void test_connection_join_busy()
{
    printf( "-----------------------------------------------------\n" );
    printf( "test connection join busy\n" );
    printf( "-----------------------------------------------------\n" );
    
    const int ServerPort = 30000;
    const int ClientPort = 30001;
    const int ProtocolId = 0x11112222;
    const float DeltaTime = 0.001f;
    const float TimeOut = 0.1f;
    
    // connect client to server
    
    Connection client( ProtocolId, TimeOut );
    Connection server( ProtocolId, TimeOut );
    
    check( client.Start( ClientPort ) );
    check( server.Start( ServerPort ) );
    
    client.Connect( Address(127,0,0,1,ServerPort ) );
    server.Listen();
    
    while ( true )
    {
        if ( client.IsConnected() && server.IsConnected() )
            break;
        
        if ( !client.IsConnecting() && client.ConnectFailed() )
            break;
        
        unsigned char client_packet[] = "client to server";
        client.SendPacket( client_packet, sizeof( client_packet ) );
        
        unsigned char server_packet[] = "server to client";
        server.SendPacket( server_packet, sizeof( server_packet ) );
        
        while ( true )
        {
            unsigned char packet[256];
            int bytes_read = client.ReceivePacket( packet, sizeof(packet) );
            if ( bytes_read == 0 )
                break;
        }
        
        while ( true )
        {
            unsigned char packet[256];
            int bytes_read = server.ReceivePacket( packet, sizeof(packet) );
            if ( bytes_read == 0 )
                break;
        }
        
        client.Update( DeltaTime );
        server.Update( DeltaTime );
    }
    
    check( client.IsConnected() );
    check( server.IsConnected() );
    
    // attempt another connection, verify connect fails (busy)
    
    Connection busy( ProtocolId, TimeOut );
    check( busy.Start( ClientPort + 1 ) );
    busy.Connect( Address(127,0,0,1,ServerPort ) );
    
    while ( true )
    {
        if ( !busy.IsConnecting() || busy.IsConnected() )
            break;
        
        unsigned char client_packet[] = "client to server";
        client.SendPacket( client_packet, sizeof( client_packet ) );
        
        unsigned char server_packet[] = "server to client";
        server.SendPacket( server_packet, sizeof( server_packet ) );
        
        unsigned char busy_packet[] = "i'm so busy!";
        busy.SendPacket( busy_packet, sizeof( busy_packet ) );
        
        while ( true )
        {
            unsigned char packet[256];
            int bytes_read = client.ReceivePacket( packet, sizeof(packet) );
            if ( bytes_read == 0 )
                break;
        }
        
        while ( true )
        {
            unsigned char packet[256];
            int bytes_read = server.ReceivePacket( packet, sizeof(packet) );
            if ( bytes_read == 0 )
                break;
        }
        
        while ( true )
        {
            unsigned char packet[256];
            int bytes_read = busy.ReceivePacket( packet, sizeof(packet) );
            if ( bytes_read == 0 )
                break;
        }
        
        client.Update( DeltaTime );
        server.Update( DeltaTime );
        busy.Update( DeltaTime );
    }
    
    check( client.IsConnected() );
    check( server.IsConnected() );
    check( !busy.IsConnected() );
    check( busy.ConnectFailed() );
}

void test_connection_rejoin()
{
    printf( "-----------------------------------------------------\n" );
    printf( "test connection rejoin\n" );
    printf( "-----------------------------------------------------\n" );
    
    const int ServerPort = 30000;
    const int ClientPort = 30001;
    const int ProtocolId = 0x11112222;
    const float DeltaTime = 0.001f;
    const float TimeOut = 0.1f;
    
    Connection client( ProtocolId, TimeOut );
    Connection server( ProtocolId, TimeOut );
    
    check( client.Start( ClientPort ) );
    check( server.Start( ServerPort ) );
    
    // connect client and server
    
    client.Connect( Address(127,0,0,1,ServerPort ) );
    server.Listen();
    
    while ( true )
    {
        if ( client.IsConnected() && server.IsConnected() )
            break;
        
        if ( !client.IsConnecting() && client.ConnectFailed() )
            break;
        
        unsigned char client_packet[] = "client to server";
        client.SendPacket( client_packet, sizeof( client_packet ) );
        
        unsigned char server_packet[] = "server to client";
        server.SendPacket( server_packet, sizeof( server_packet ) );
        
        while ( true )
        {
            unsigned char packet[256];
            int bytes_read = client.ReceivePacket( packet, sizeof(packet) );
            if ( bytes_read == 0 )
                break;
        }
        
        while ( true )
        {
            unsigned char packet[256];
            int bytes_read = server.ReceivePacket( packet, sizeof(packet) );
            if ( bytes_read == 0 )
                break;
        }
        
        client.Update( DeltaTime );
        server.Update( DeltaTime );
    }
    
    check( client.IsConnected() );
    check( server.IsConnected() );
    
    // let connection timeout
    
    while ( client.IsConnected() || server.IsConnected() )
    {
        while ( true )
        {
            unsigned char packet[256];
            int bytes_read = client.ReceivePacket( packet, sizeof(packet) );
            if ( bytes_read == 0 )
                break;
        }
        
        while ( true )
        {
            unsigned char packet[256];
            int bytes_read = server.ReceivePacket( packet, sizeof(packet) );
            if ( bytes_read == 0 )
                break;
        }
        
        client.Update( DeltaTime );
        server.Update( DeltaTime );
    }
    
    check( !client.IsConnected() );
    check( !server.IsConnected() );
    
    // reconnect client
    
    client.Connect( Address(127,0,0,1,ServerPort ) );
    
    while ( true )
    {
        if ( client.IsConnected() && server.IsConnected() )
            break;
        
        if ( !client.IsConnecting() && client.ConnectFailed() )
            break;
        
        unsigned char client_packet[] = "client to server";
        client.SendPacket( client_packet, sizeof( client_packet ) );
        
        unsigned char server_packet[] = "server to client";
        server.SendPacket( server_packet, sizeof( server_packet ) );
        
        while ( true )
        {
            unsigned char packet[256];
            int bytes_read = client.ReceivePacket( packet, sizeof(packet) );
            if ( bytes_read == 0 )
                break;
        }
        
        while ( true )
        {
            unsigned char packet[256];
            int bytes_read = server.ReceivePacket( packet, sizeof(packet) );
            if ( bytes_read == 0 )
                break;
        }
        
        client.Update( DeltaTime );
        server.Update( DeltaTime );
    }
    
    check( client.IsConnected() );
    check( server.IsConnected() );
}

void test_connection_payload()
{
    printf( "-----------------------------------------------------\n" );
    printf( "test connection payload\n" );
    printf( "-----------------------------------------------------\n" );
    
    const int ServerPort = 30000;
    const int ClientPort = 30001;
    const int ProtocolId = 0x11112222;
    const float DeltaTime = 0.001f;
    const float TimeOut = 0.1f;
    
    Connection client( ProtocolId, TimeOut );
    Connection server( ProtocolId, TimeOut );
    
    check( client.Start( ClientPort ) );
    check( server.Start( ServerPort ) );
    
    client.Connect( Address(127,0,0,1,ServerPort ) );
    server.Listen();
    
    while ( true )
    {
        if ( client.IsConnected() && server.IsConnected() )
            break;
        
        if ( !client.IsConnecting() && client.ConnectFailed() )
            break;
        
        unsigned char client_packet[] = "client to server";
        client.SendPacket( client_packet, sizeof( client_packet ) );
        
        unsigned char server_packet[] = "server to client";
        server.SendPacket( server_packet, sizeof( server_packet ) );
        
        while ( true )
        {
            unsigned char packet[256];
            int bytes_read = client.ReceivePacket( packet, sizeof(packet) );
            if ( bytes_read == 0 )
                break;
            check( strcmp( (const char*) packet, "server to client" ) == 0 );
        }
        
        while ( true )
        {
            unsigned char packet[256];
            int bytes_read = server.ReceivePacket( packet, sizeof(packet) );
            if ( bytes_read == 0 )
                break;
            check( strcmp( (const char*) packet, "client to server" ) == 0 );
        }
        
        client.Update( DeltaTime );
        server.Update( DeltaTime );
    }
    
    check( client.IsConnected() );
    check( server.IsConnected() );
}


void RunConnectionTests()
{
    printf( "-----------------------------------------------------\n" );
    printf( "running connection tests...\n" );

    test_connection_join();
    test_connection_join_timeout();
    test_connection_join_busy();
    test_connection_rejoin();
    test_connection_payload();
    
    printf( "-----------------------------------------------------\n" );
    printf( "connection tests passed!\n" );
}