//
//  TransportLANTests.cpp
//  DrudgeNet
//
//  Created by The Drudgerist on 26/12/15.
//  Copyright © 2015 The Drudgerist. All rights reserved.
//

#include "TransportLANTests.hpp"
#include "Connection.h"
#include "ReliableConnection.h"
#include "PacketQueue.h"
#include "Mesh.h"
#include "Node.h"
#include "TransportLAN.h"

#include <cassert>
#include <string>

// -------------------------------------------------------------------------------
// unit tests for transport layer
// -------------------------------------------------------------------------------

using namespace Net;

#ifdef DEBUG
#define check assert
#else
#define check(n) if ( !n ) { printf( "check failed\n" ); exit(1); }
#endif

void test_lan_transport_connect()
{
    printf( "-----------------------------------------------------\n" );
    printf( "test LAN transport join\n" );
    printf( "-----------------------------------------------------\n" );
    const float DeltaTime = 1.0f / 30.0f;

    // create transports
    Transport * server = Transport::Create();
    check( server != nullptr );
    
    Transport * client = Transport::Create();
    check( client != nullptr );
    
    // Setup server transport
    TransportLAN * lan_transport_server = dynamic_cast<TransportLAN*>( server );
    std::string hostname = "testhostname";
    lan_transport_server->StartServer( hostname.c_str() );
    
    // try to find the server
    TransportLAN * lan_transport_client = dynamic_cast<TransportLAN*>( client );
    lan_transport_client->EnterLobby();
    
    while ( true )
    {
        if ( lan_transport_client->GetLobbyEntryCount() )
            break;
        
        client->Update( DeltaTime );
        server->Update( DeltaTime );
    }
    TransportLAN::LobbyEntry entry;
    lan_transport_client->GetLobbyEntryAtIndex(0, entry);
    
    lan_transport_client->Stop();

    printf( "client found host: %s\n", entry.name );

    // connect to server (transport specific)
    lan_transport_client->ConnectClient( entry.name );
    
    while ( true )
    {
        if ( lan_transport_client->IsConnected() && lan_transport_server->IsConnected() )
            break;
        
        if ( lan_transport_client->ConnectFailed() )
            break;
        
        server->Update( DeltaTime );
        client->Update( DeltaTime );
    }
    
    check( lan_transport_client->IsConnected() );
    check( lan_transport_server->IsConnected() );
    
    // shutdown
    
    Transport::Destroy( client );
    Transport::Destroy( server );
}

void test_lan_transport_connect_fail()
{
    printf( "-----------------------------------------------------\n" );
    printf( "test LAN transport connect fail\n" );
    printf( "-----------------------------------------------------\n" );
    
    // create transport
    Transport * client = Transport::Create();
    check( client != nullptr );
    
    std::string hostname = "testhostname";
    
    // try to connect to server (transport specific)
    TransportLAN * lan_transport_client = dynamic_cast<TransportLAN*>( client );
    lan_transport_client->ConnectClient( hostname.c_str() );
    
    const float DeltaTime = 1.0f / 30.0f;
    
    while ( true )
    {
        if ( lan_transport_client->IsConnected() )
            break;
        
        if ( lan_transport_client->ConnectFailed() )
            break;
        
        client->Update( DeltaTime );
    }
    
    check( lan_transport_client->ConnectFailed() );
    
    // shutdown
    
    Transport::Destroy( client );
}

void test_lan_transport_connect_busy()
{
    printf( "-----------------------------------------------------\n" );
    printf( "test LAN transport connect busy\n" );
    printf( "-----------------------------------------------------\n" );
    
    // create transports
    Transport * server = Transport::Create();
    check( server != nullptr );
    
    Transport * clientA = Transport::Create();
    check( clientA != nullptr );
    
    Transport * clientB = Transport::Create();
    check( clientB != nullptr );
    
    // Setup server transport
    TransportLAN * lan_transport_server = dynamic_cast<TransportLAN*>( server );
    std::string hostname = "testhostname";
    TransportLAN::Config config;
//    config.meshPort = 30000;
//    config.clientPort = 30001;
//    config.serverPort = 30002;
//    config.beaconPort = 40000;
//    config.listenerPort = 40001;
//    config.protocolId = 0x12345678;
//    config.meshSendRate = 0.25f;
//    config.timeout = 10.0f;
    config.maxNodes = 2;
    
    lan_transport_server->Configure(config);
    lan_transport_server->StartServer( hostname.c_str() );
    
    // connect client A to server (transport specific)
    TransportLAN * lan_transport_clientA = dynamic_cast<TransportLAN*>( clientA );
    lan_transport_clientA->ConnectClient( hostname.c_str() );
    
    const float DeltaTime = 1.0f / 30.0f;
    
    while ( true )
    {
        if ( lan_transport_clientA->IsConnected() && lan_transport_server->IsConnected() )
            break;
        
        if ( lan_transport_clientA->ConnectFailed() )
            break;
        
        clientA->Update( DeltaTime );
        server->Update( DeltaTime );
    }
    
    check( lan_transport_clientA->IsConnected() );
    check( lan_transport_server->IsConnected() );
    
    // try to connect client B
    TransportLAN * lan_transport_clientB = dynamic_cast<TransportLAN*>( clientB );
    config.clientPort = 60001;
    config.serverPort = 30002;
    config.beaconPort = 40000;
    config.listenerPort = 60001;
    
    lan_transport_clientB->Configure(config);
    lan_transport_clientB->ConnectClient( hostname.c_str() );
    while ( true )
    {
        if ( lan_transport_clientB->IsConnected() )
            break;
        
        if ( lan_transport_clientB->ConnectFailed() )
            break;
        
        clientA->Update( DeltaTime );
        clientB->Update( DeltaTime );
        server->Update( DeltaTime );
    }
    
    check( lan_transport_clientA->IsConnected() );
    check( lan_transport_clientB->ConnectFailed() );
    check( lan_transport_server->IsConnected() );
    
    // shutdown
    
    Transport::Destroy( clientA );
    Transport::Destroy( clientB );
    Transport::Destroy( server );
}

void test_lan_transport_reconnect()
{
    printf( "-----------------------------------------------------\n" );
    printf( "test LAN transport reconnect\n" );
    printf( "-----------------------------------------------------\n" );
    
    // create transports
    Transport * server = Transport::Create();
    check( server != nullptr );
    
    Transport * client = Transport::Create();
    check( client != nullptr );
    
    // Setup server transport
    TransportLAN * lan_transport_server = dynamic_cast<TransportLAN*>( server );
    std::string hostname = "testhostname";
    lan_transport_server->StartServer( hostname.c_str() );
    
    // connect to server (transport specific)
    TransportLAN * lan_transport_client = dynamic_cast<TransportLAN*>( client );
    lan_transport_client->ConnectClient( hostname.c_str() );
    
    const float DeltaTime = 1.0f / 30.0f;
    
    while ( true )
    {
        if ( lan_transport_client->IsConnected() && lan_transport_server->IsConnected() )
            break;
        
        if ( lan_transport_client->ConnectFailed() )
            break;
        
        client->Update( DeltaTime );
        server->Update( DeltaTime );
    }
    
    check( lan_transport_client->IsConnected() );
    check( lan_transport_server->IsConnected() );
    
    // disconnect client node
    int firstClientNodeID = lan_transport_client->GetLocalNodeId();
    lan_transport_client->Stop();
    
    // make sure client node times out from mesh
    while ( true )
    {
        if ( !server->IsNodeConnected(firstClientNodeID) )
            break;
        
        server->Update( DeltaTime );
    }
    
    check( !lan_transport_client->IsConnected() );
    check( !server->IsNodeConnected(firstClientNodeID) );
    
    // reconnect
    lan_transport_client->ConnectClient( hostname.c_str() );
    
    while ( true )
    {
        if ( lan_transport_client->IsConnected() && lan_transport_server->IsConnected() )
            break;
        
        if ( lan_transport_client->ConnectFailed() )
            break;
        
        client->Update( DeltaTime );
        server->Update( DeltaTime );
    }
    
    check( lan_transport_client->IsConnected() );
    check( lan_transport_server->IsConnected() );
    check( lan_transport_client->GetLocalNodeId() == firstClientNodeID );
    
    // shutdown
    
    Transport::Destroy( client );
    Transport::Destroy( server );
}

void test_lan_transport_client_server()
{
    printf( "-----------------------------------------------------\n" );
    printf( "test LAN transport client-server\n" );
    printf( "-----------------------------------------------------\n" );
    const float DeltaTime = 1.0f / 30.0f;
    
    // create transports
    Transport * server = Transport::Create();
    check( server != nullptr );
    
    Transport * client = Transport::Create();
    check( client != nullptr );
    
    // Setup server transport
    TransportLAN * lan_transport_server = dynamic_cast<TransportLAN*>( server );
    std::string hostname = "testhostname";
    lan_transport_server->StartServer( hostname.c_str() );
    
    // connect to server (transport specific)
    TransportLAN * lan_transport_client = dynamic_cast<TransportLAN*>( client );
    lan_transport_client->ConnectClient( hostname.c_str() );
    
    bool serverReceivedPacketFromClient = false;
    bool clientReceivedPacketFromServer = false;

    while ( !serverReceivedPacketFromClient || !clientReceivedPacketFromServer )
    {
        if ( lan_transport_client->IsConnected() )
        {
            unsigned char packet[] = "client to server";
            client->SendPacket( 0, packet, sizeof(packet) );
        }
        
        if ( lan_transport_server->IsConnected() )
        {
            unsigned char packet[] = "server to client";
            server->SendPacket( 1, packet, sizeof(packet) );
        }
        
        if ( lan_transport_client->IsConnected() )
        {
            while ( true )
            {
                int nodeId = -1;
                unsigned char packet[256];
                int bytes_read = client->ReceivePacket( nodeId, packet, sizeof(packet) );
                if ( bytes_read == 0 )
                    break;
                printf("client received %i bytes from node %i\n", bytes_read, nodeId);

                if ( nodeId == 0 && strcmp( (const char*) packet, "server to client" ) == 0 )
                    clientReceivedPacketFromServer = true;
            }
        }
        
        if ( lan_transport_server->IsConnected() )
        {
            while ( true )
            {
                int nodeId = -1;
                unsigned char packet[256];
                int bytes_read = server->ReceivePacket( nodeId, packet, sizeof(packet) );
                if ( bytes_read == 0 )
                    break;
                printf("server received %i bytes from node %i\n", bytes_read, nodeId);

                if ( nodeId == 1 && strcmp( (const char*) packet, "client to server" ) == 0 )
                    serverReceivedPacketFromClient = true;
            }
        }

        client->Update( DeltaTime );
        server->Update( DeltaTime );
    }
    
    check( lan_transport_client->IsConnected() );
    check( lan_transport_server->IsConnected() );
    
    // shutdown
    
    Transport::Destroy( client );
    Transport::Destroy( server );
}

void test_lan_transport_peer_to_peer()
{
    printf( "-----------------------------------------------------\n" );
    printf( "test LAN transport peer-to-peer\n" );
    printf( "-----------------------------------------------------\n" );
    
    // create transports
    Transport * server = Transport::Create();
    check( server != nullptr );
    
    Transport * clientA = Transport::Create();
    check( clientA != nullptr );
    
    Transport * clientB = Transport::Create();
    check( clientB != nullptr );
    
    // Setup server transport
    TransportLAN * lan_transport_server = dynamic_cast<TransportLAN*>( server );
    std::string hostname = "testhostname";
    lan_transport_server->StartServer( hostname.c_str() );
    
    // connect client A to server (transport specific)
    TransportLAN * lan_transport_clientA = dynamic_cast<TransportLAN*>( clientA );
    lan_transport_clientA->ConnectClient( hostname.c_str() );
    
    const float DeltaTime = 1.0f / 30.0f;
    
    while ( true )
    {
        if (lan_transport_clientA->IsConnected() &&
            lan_transport_server->IsConnected() )
            break;
        
        if (lan_transport_clientA->ConnectFailed())
            break;
        
        clientA->Update( DeltaTime );
        server->Update( DeltaTime );
    }
    
    check( lan_transport_clientA->IsConnected() );
    check( lan_transport_server->IsConnected() );
    
    // connect client B to server, done in separate pass to reuse listener port :/
    TransportLAN::Config config;
    config.clientPort = 30003;
    TransportLAN * lan_transport_clientB = dynamic_cast<TransportLAN*>( clientB );
    lan_transport_clientB->Configure(config);
    lan_transport_clientB->ConnectClient( hostname.c_str() );
    
    bool clientAReceivedPacketFromServer = false;
    bool clientBReceivedPacketFromServer = false;
    
    while ( !clientAReceivedPacketFromServer || !clientBReceivedPacketFromServer )
    {
        if (lan_transport_clientA->IsConnected() &&
            lan_transport_clientB->IsConnected())
        {
            unsigned char packetA[] = "client A to client B";
            clientA->SendPacket( clientB->GetLocalNodeId(), packetA, sizeof(packetA) );
            unsigned char packetB[] = "client B to client A";
            clientB->SendPacket( clientA->GetLocalNodeId(), packetB, sizeof(packetB) );
        }
        
        if ( lan_transport_clientA->IsConnected() )
        {
            while ( true )
            {
                int nodeId = -1;
                unsigned char packet[256];
                int bytes_read = clientA->ReceivePacket( nodeId, packet, sizeof(packet) );
                if ( bytes_read == 0 )
                    break;
                if (nodeId == clientB->GetLocalNodeId() &&
                    strcmp( (const char*) packet, "client B to client A" ) == 0 )
                    clientAReceivedPacketFromServer = true;
            }
        }
        
        if ( lan_transport_clientB->IsConnected() )
        {
            while ( true )
            {
                int nodeId = -1;
                unsigned char packet[256];
                int bytes_read = clientB->ReceivePacket( nodeId, packet, sizeof(packet) );
                if ( bytes_read == 0 )
                    break;
                if (nodeId == clientA->GetLocalNodeId() &&
                    strcmp( (const char*) packet, "client A to client B" ) == 0 )
                    clientBReceivedPacketFromServer = true;
            }
        }
        
        clientA->Update( DeltaTime );
        clientB->Update( DeltaTime );
        server->Update( DeltaTime );
    }
    
    check( lan_transport_clientA->IsConnected() );
    check( lan_transport_clientB->IsConnected() );
    check( lan_transport_server->IsConnected() );
    
    // shutdown
    
    Transport::Destroy( clientA );
    Transport::Destroy( clientB );
    Transport::Destroy( server );
}

void test_lan_transport_reliability()
{
    printf( "-----------------------------------------------------\n" );
    printf( "test LAN transport reliability\n" );
    printf( "-----------------------------------------------------\n" );

    // create transports
    Transport * server = Transport::Create();
    check( server != nullptr );
    
    Transport * client = Transport::Create();
    check( client != nullptr );
    
    // setup server transport
    TransportLAN * lan_transport_server = dynamic_cast<TransportLAN*>( server );
    std::string hostname = "testhostname";
    lan_transport_server->StartServer( hostname.c_str() );
    
    // connect to mesh
    TransportLAN * lan_transport_client = dynamic_cast<TransportLAN*>( client );
    lan_transport_client->ConnectClient( "127.0.0.1:30000" );
    
    const float DeltaTime = 1.0f / 30.0f;
    const unsigned int PacketCount = 100;
    
    bool clientAckedPackets[PacketCount];
    bool serverAckedPackets[PacketCount];
    for ( unsigned int i = 0; i < PacketCount; ++i )
    {
        clientAckedPackets[i] = false;
        serverAckedPackets[i] = false;
    }
    
    bool allPacketsAcked = false;

    while ( true )
    {
        if (lan_transport_client->ConnectFailed())
            break;
        
        if ( allPacketsAcked )
            break;
        
        unsigned char packet[256];
        for ( unsigned int i = 0; i < sizeof(packet); ++i )
            packet[i] = (unsigned char) i;
        
//        if (lan_transport_client->IsConnected() &&
//            lan_transport_server->IsConnected() &&
//            client->IsNodeConnected(1) &&
//            client->IsNodeConnected(0) &&
//            server->IsNodeConnected(1) &&
//            server->IsNodeConnected(0))
        {
            server->SendPacket( 1, packet, sizeof(packet) );
            client->SendPacket( 0, packet, sizeof(packet) );

            // client reads data from server
            while ( true )
            {
                int nodeId = -1;
                unsigned char packet[256];
                int bytes_read = client->ReceivePacket( nodeId, packet, sizeof(packet) );
                if ( bytes_read == 0 )
                    break;
                check( bytes_read == sizeof(packet) );
                for ( unsigned int i = 0; i < sizeof(packet); ++i )
                    check( packet[i] == (unsigned char) i );
            }
            
            // server reads data from client
            while ( true )
            {
                int nodeId = -1;
                unsigned char packet[256];
                int bytes_read = server->ReceivePacket( nodeId, packet, sizeof(packet) );
                if ( bytes_read == 0 )
                    break;
                check( bytes_read == sizeof(packet) );
                for ( unsigned int i = 0; i < sizeof(packet); ++i )
                    check( packet[i] == (unsigned char) i );
            }
            
            int ack_count = 0;
            unsigned int * acks = NULL;
            lan_transport_client->GetReliability(0).GetAcks( &acks, ack_count );
            check( ack_count == 0 || ack_count != 0 && acks );
            for ( int i = 0; i < ack_count; ++i )
            {
                unsigned int ack = acks[i];
                if ( ack < PacketCount )
                {
                    check( clientAckedPackets[ack] == false );
                    clientAckedPackets[ack] = true;
                }
            }
            
            lan_transport_server->GetReliability(1).GetAcks( &acks, ack_count );
            check( ack_count == 0 || ack_count != 0 && acks );
            for ( int i = 0; i < ack_count; ++i )
            {
                unsigned int ack = acks[i];
                if ( ack < PacketCount )
                {
                    check( serverAckedPackets[ack] == false );
                    serverAckedPackets[ack] = true;
                }
            }
            
            unsigned int clientAckCount = 0;
            unsigned int serverAckCount = 0;
            for ( unsigned int i = 0; i < PacketCount; ++i )
            {
                clientAckCount += clientAckedPackets[i];
                serverAckCount += serverAckedPackets[i];
            }
            allPacketsAcked = clientAckCount == PacketCount && serverAckCount == PacketCount;
        }
        
        client->Update( DeltaTime );
        server->Update( DeltaTime );
    }
    
    check( lan_transport_client->IsConnected() );
    check( lan_transport_server->IsConnected() );
    
    // shutdown
    
    Transport::Destroy( client );
    Transport::Destroy( server );
}

void RunTransportTests()
{
    printf( "-----------------------------------------------------\n" );
    printf( "running transport tests...\n" );
    
    check( InitializeSockets() );
    
    
    TransportType type = Transport_LAN;
    check( Transport::Initialize( type ) );
    
    test_lan_transport_connect();
    test_lan_transport_connect_fail();
    test_lan_transport_connect_busy();
    test_lan_transport_reconnect();
    test_lan_transport_client_server();
    test_lan_transport_peer_to_peer();
    test_lan_transport_reliability();
    
    Transport::Shutdown();
    
    ShutdownSockets();
    
    printf( "-----------------------------------------------------\n" );
    printf( "transport tests passed!\n" );
}
