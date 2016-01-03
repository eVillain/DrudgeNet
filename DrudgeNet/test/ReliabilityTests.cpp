//
//  ReliabilityTests.cpp
//  DrudgeNet
//
//  Created by The Drudgerist on 25/12/15.
//  Copyright © 2015 The Drudgerist. All rights reserved.
//

#include "ReliabilityTests.hpp"
#include "ReliableConnection.h"
#include <cassert>
#include <string>
#include <stdio.h>

using namespace Net;

#ifdef DEBUG
#define check assert
#else
#define check(n) if ( !(n) ) { printf( "check failed\n" ); exit(1); }
#endif

void test_packet_queue()
{
    printf( "-----------------------------------------------------\n" );
    printf( "test packet queue\n" );
    printf( "-----------------------------------------------------\n" );
    
    const unsigned int MaximumSequence = 255;
    
    PacketQueue packetQueue;
    
    printf( "check insert back\n" );
    for ( int i = 0; i < 100; ++i )
    {
        PacketData data;
        data.sequence = i;
        packetQueue.InsertSorted( data, MaximumSequence );
        packetQueue.VerifySorted( MaximumSequence );
    }
    
    printf( "check insert front\n" );
    packetQueue.clear();
    for ( int i = 100; i < 0; ++i )
    {
        PacketData data;
        data.sequence = i;
        packetQueue.InsertSorted( data, MaximumSequence );
        packetQueue.VerifySorted( MaximumSequence );
    }
    
    printf( "check insert random\n" );
    packetQueue.clear();
    for ( int i = 100; i < 0; ++i )
    {
        PacketData data;
        data.sequence = rand() & 0xFF;
        packetQueue.InsertSorted( data, MaximumSequence );
        packetQueue.VerifySorted( MaximumSequence );
    }
    
    printf( "check insert wrap around\n" );
    packetQueue.clear();
    for ( int i = 200; i <= 255; ++i )
    {
        PacketData data;
        data.sequence = i;
        packetQueue.InsertSorted( data, MaximumSequence );
        packetQueue.VerifySorted( MaximumSequence );
    }
    for ( int i = 0; i <= 50; ++i )
    {
        PacketData data;
        data.sequence = i;
        packetQueue.InsertSorted( data, MaximumSequence );
        packetQueue.VerifySorted( MaximumSequence );
    }
}

void test_reliability_system()
{
    printf( "-----------------------------------------------------\n" );
    printf( "test reliability system\n" );
    printf( "-----------------------------------------------------\n" );
    
    const int MaximumSequence = 255;
    
    printf( "check bit index for sequence\n" );
    check( ReliabilitySystem::BitIndexForSequence( 99, 100, MaximumSequence ) == 0 );
    check( ReliabilitySystem::BitIndexForSequence( 90, 100, MaximumSequence ) == 9 );
    check( ReliabilitySystem::BitIndexForSequence( 0, 1, MaximumSequence ) == 0 );
    check( ReliabilitySystem::BitIndexForSequence( 255, 0, MaximumSequence ) == 0 );
    check( ReliabilitySystem::BitIndexForSequence( 255, 1, MaximumSequence ) == 1 );
    check( ReliabilitySystem::BitIndexForSequence( 254, 1, MaximumSequence ) == 2 );
    check( ReliabilitySystem::BitIndexForSequence( 254, 2, MaximumSequence ) == 3 );
    
    printf( "check generate ack bits\n");
    PacketQueue packetQueue;
    for ( int i = 0; i < 32; ++i )
    {
        PacketData data;
        data.sequence = i;
        packetQueue.InsertSorted( data, MaximumSequence );
        packetQueue.VerifySorted( MaximumSequence );
    }
    check( ReliabilitySystem::GenerateAckBits( 32, packetQueue, MaximumSequence ) == 0xFFFFFFFF );
    check( ReliabilitySystem::GenerateAckBits( 31, packetQueue, MaximumSequence ) == 0x7FFFFFFF );
    check( ReliabilitySystem::GenerateAckBits( 33, packetQueue, MaximumSequence ) == 0xFFFFFFFE );
    check( ReliabilitySystem::GenerateAckBits( 16, packetQueue, MaximumSequence ) == 0x0000FFFF );
    check( ReliabilitySystem::GenerateAckBits( 48, packetQueue, MaximumSequence ) == 0xFFFF0000 );
    
    printf( "check generate ack bits with wrap\n");
    packetQueue.clear();
    for ( int i = 255 - 31; i <= 255; ++i )
    {
        PacketData data;
        data.sequence = i;
        packetQueue.InsertSorted( data, MaximumSequence );
        packetQueue.VerifySorted( MaximumSequence );
    }
    check( packetQueue.size() == 32 );
    check( ReliabilitySystem::GenerateAckBits( 0, packetQueue, MaximumSequence ) == 0xFFFFFFFF );
    check( ReliabilitySystem::GenerateAckBits( 255, packetQueue, MaximumSequence ) == 0x7FFFFFFF );
    check( ReliabilitySystem::GenerateAckBits( 1, packetQueue, MaximumSequence ) == 0xFFFFFFFE );
    check( ReliabilitySystem::GenerateAckBits( 240, packetQueue, MaximumSequence ) == 0x0000FFFF );
    check( ReliabilitySystem::GenerateAckBits( 16, packetQueue, MaximumSequence ) == 0xFFFF0000 );
    
    printf( "check process ack (1)\n" );
    {
        PacketQueue pendingAckQueue;
        for ( int i = 0; i < 33; ++i )
        {
            PacketData data;
            data.sequence = i;
            data.time = 0.0f;
            pendingAckQueue.InsertSorted( data, MaximumSequence );
            pendingAckQueue.VerifySorted( MaximumSequence );
        }
        PacketQueue ackedQueue;
        std::vector<unsigned int> acks;
        float rtt = 0.0f;
        unsigned int acked_packets = 0;
        ReliabilitySystem::ProcessAck( 32, 0xFFFFFFFF, pendingAckQueue, ackedQueue, acks, acked_packets, rtt, MaximumSequence );
        check( acks.size() == 33 );
        check( acked_packets == 33 );
        check( ackedQueue.size() == 33 );
        check( pendingAckQueue.size() == 0 );
        ackedQueue.VerifySorted( MaximumSequence );
        for ( unsigned int i = 0; i < acks.size(); ++i )
            check( acks[i] == i );
        unsigned int i = 0;
        for ( PacketQueue::iterator itor = ackedQueue.begin(); itor != ackedQueue.end(); ++itor, ++i )
            check( itor->sequence == i );
    }
    
    printf( "check process ack (2)\n" );
    {
        PacketQueue pendingAckQueue;
        for ( int i = 0; i < 33; ++i )
        {
            PacketData data;
            data.sequence = i;
            data.time = 0.0f;
            pendingAckQueue.InsertSorted( data, MaximumSequence );
            pendingAckQueue.VerifySorted( MaximumSequence );
        }
        PacketQueue ackedQueue;
        std::vector<unsigned int> acks;
        float rtt = 0.0f;
        unsigned int acked_packets = 0;
        ReliabilitySystem::ProcessAck( 32, 0x0000FFFF, pendingAckQueue, ackedQueue, acks, acked_packets, rtt, MaximumSequence );
        check( acks.size() == 17 );
        check( acked_packets == 17 );
        check( ackedQueue.size() == 17 );
        check( pendingAckQueue.size() == 33 - 17 );
        ackedQueue.VerifySorted( MaximumSequence );
        unsigned int i = 0;
        for ( PacketQueue::iterator itor = pendingAckQueue.begin(); itor != pendingAckQueue.end(); ++itor, ++i )
            check( itor->sequence == i );
        i = 0;
        for ( PacketQueue::iterator itor = ackedQueue.begin(); itor != ackedQueue.end(); ++itor, ++i )
            check( itor->sequence == i + 16 );
        for ( unsigned int i = 0; i < acks.size(); ++i )
            check( acks[i] == i + 16 );
    }
    
    printf( "check process ack (3)\n" );
    {
        PacketQueue pendingAckQueue;
        for ( int i = 0; i < 32; ++i )
        {
            PacketData data;
            data.sequence = i;
            data.time = 0.0f;
            pendingAckQueue.InsertSorted( data, MaximumSequence );
            pendingAckQueue.VerifySorted( MaximumSequence );
        }
        PacketQueue ackedQueue;
        std::vector<unsigned int> acks;
        float rtt = 0.0f;
        unsigned int acked_packets = 0;
        ReliabilitySystem::ProcessAck( 48, 0xFFFF0000, pendingAckQueue, ackedQueue, acks, acked_packets, rtt, MaximumSequence );
        check( acks.size() == 16 );
        check( acked_packets == 16 );
        check( ackedQueue.size() == 16 );
        check( pendingAckQueue.size() == 16 );
        ackedQueue.VerifySorted( MaximumSequence );
        unsigned int i = 0;
        for ( PacketQueue::iterator itor = pendingAckQueue.begin(); itor != pendingAckQueue.end(); ++itor, ++i )
            check( itor->sequence == i );
        i = 0;
        for ( PacketQueue::iterator itor = ackedQueue.begin(); itor != ackedQueue.end(); ++itor, ++i )
            check( itor->sequence == i + 16 );
        for ( unsigned int i = 0; i < acks.size(); ++i )
            check( acks[i] == i + 16 );
    }
    
    printf( "check process ack wrap around (1)\n" );
    {
        PacketQueue pendingAckQueue;
        for ( int i = 255 - 31; i <= 256; ++i )
        {
            PacketData data;
            data.sequence = i & 0xFF;
            data.time = 0.0f;
            pendingAckQueue.InsertSorted( data, MaximumSequence );
            pendingAckQueue.VerifySorted( MaximumSequence );
        }
        check( pendingAckQueue.size() == 33 );
        PacketQueue ackedQueue;
        std::vector<unsigned int> acks;
        float rtt = 0.0f;
        unsigned int acked_packets = 0;
        ReliabilitySystem::ProcessAck( 0, 0xFFFFFFFF, pendingAckQueue, ackedQueue, acks, acked_packets, rtt, MaximumSequence );
        check( acks.size() == 33 );
        check( acked_packets == 33 );
        check( ackedQueue.size() == 33 );
        check( pendingAckQueue.size() == 0 );
        ackedQueue.VerifySorted( MaximumSequence );
        for ( unsigned int i = 0; i < acks.size(); ++i )
            check( acks[i] == ( (i+255-31) & 0xFF ) );
        unsigned int i = 0;
        for ( PacketQueue::iterator itor = ackedQueue.begin(); itor != ackedQueue.end(); ++itor, ++i )
            check( itor->sequence == ( (i+255-31) & 0xFF ) );
    }
    
    printf( "check process ack wrap around (2)\n" );
    {
        PacketQueue pendingAckQueue;
        for ( int i = 255 - 31; i <= 256; ++i )
        {
            PacketData data;
            data.sequence = i & 0xFF;
            data.time = 0.0f;
            pendingAckQueue.InsertSorted( data, MaximumSequence );
            pendingAckQueue.VerifySorted( MaximumSequence );
        }
        check( pendingAckQueue.size() == 33 );
        PacketQueue ackedQueue;
        std::vector<unsigned int> acks;
        float rtt = 0.0f;
        unsigned int acked_packets = 0;
        ReliabilitySystem::ProcessAck( 0, 0x0000FFFF, pendingAckQueue, ackedQueue, acks, acked_packets, rtt, MaximumSequence );
        check( acks.size() == 17 );
        check( acked_packets == 17 );
        check( ackedQueue.size() == 17 );
        check( pendingAckQueue.size() == 33 - 17 );
        ackedQueue.VerifySorted( MaximumSequence );
        for ( unsigned int i = 0; i < acks.size(); ++i )
            check( acks[i] == ( (i+255-15) & 0xFF ) );
        unsigned int i = 0;
        for ( PacketQueue::iterator itor = pendingAckQueue.begin(); itor != pendingAckQueue.end(); ++itor, ++i )
            check( itor->sequence == i + 255 - 31 );
        i = 0;
        for ( PacketQueue::iterator itor = ackedQueue.begin(); itor != ackedQueue.end(); ++itor, ++i )
            check( itor->sequence == ( (i+255-15) & 0xFF ) );
    }
    
    printf( "check process ack wrap around (3)\n" );
    {
        PacketQueue pendingAckQueue;
        for ( int i = 255 - 31; i <= 255; ++i )
        {
            PacketData data;
            data.sequence = i & 0xFF;
            data.time = 0.0f;
            pendingAckQueue.InsertSorted( data, MaximumSequence );
            pendingAckQueue.VerifySorted( MaximumSequence );
        }
        check( pendingAckQueue.size() == 32 );
        PacketQueue ackedQueue;
        std::vector<unsigned int> acks;
        float rtt = 0.0f;
        unsigned int acked_packets = 0;
        ReliabilitySystem::ProcessAck( 16, 0xFFFF0000, pendingAckQueue, ackedQueue, acks, acked_packets, rtt, MaximumSequence );
        check( acks.size() == 16 );
        check( acked_packets == 16 );
        check( ackedQueue.size() == 16 );
        check( pendingAckQueue.size() == 16 );
        ackedQueue.VerifySorted( MaximumSequence );
        for ( unsigned int i = 0; i < acks.size(); ++i )
            check( acks[i] == ( (i+255-15) & 0xFF ) );
        unsigned int i = 0;
        for ( PacketQueue::iterator itor = pendingAckQueue.begin(); itor != pendingAckQueue.end(); ++itor, ++i )
            check( itor->sequence == i + 255 - 31 );
        i = 0;
        for ( PacketQueue::iterator itor = ackedQueue.begin(); itor != ackedQueue.end(); ++itor, ++i )
            check( itor->sequence == ( (i+255-15) & 0xFF ) );
    }
}

// --------------------------------------------------------

void test_reliable_connection_join()
{
    printf( "-----------------------------------------------------\n" );
    printf( "test reliable connection join\n" );
    printf( "-----------------------------------------------------\n" );
    
    const int ServerPort = 30000;
    const int ClientPort = 30001;
    const int ProtocolId = 0x11112222;
    const float DeltaTime = 0.001f;
    const float TimeOut = 1.0f;
    
    ReliableConnection client( ProtocolId, TimeOut );
    ReliableConnection server( ProtocolId, TimeOut );
    
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

void test_reliable_connection_join_timeout()
{
    printf( "-----------------------------------------------------\n" );
    printf( "test reliable connection join timeout\n" );
    printf( "-----------------------------------------------------\n" );
    
    const int ServerPort = 30000;
    const int ClientPort = 30001;
    const int ProtocolId = 0x11112222;
    const float DeltaTime = 0.001f;
    const float TimeOut = 0.1f;
    
    ReliableConnection client( ProtocolId, TimeOut );
    
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

void test_reliable_connection_join_busy()
{
    printf( "-----------------------------------------------------\n" );
    printf( "test reliable connection join busy\n" );
    printf( "-----------------------------------------------------\n" );
    
    const int ServerPort = 30000;
    const int ClientPort = 30001;
    const int ProtocolId = 0x11112222;
    const float DeltaTime = 0.001f;
    const float TimeOut = 0.1f;
    
    // connect client to server
    
    ReliableConnection client( ProtocolId, TimeOut );
    ReliableConnection server( ProtocolId, TimeOut );
    
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
    
    ReliableConnection busy( ProtocolId, TimeOut );
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

void test_reliable_connection_rejoin()
{
    printf( "-----------------------------------------------------\n" );
    printf( "test reliable connection rejoin\n" );
    printf( "-----------------------------------------------------\n" );
    
    const int ServerPort = 30000;
    const int ClientPort = 30001;
    const int ProtocolId = 0x11112222;
    const float DeltaTime = 0.001f;
    const float TimeOut = 0.1f;
    
    ReliableConnection client( ProtocolId, TimeOut );
    ReliableConnection server( ProtocolId, TimeOut );
    
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

void test_reliable_connection_payload()
{
    printf( "-----------------------------------------------------\n" );
    printf( "test reliable connection payload\n" );
    printf( "-----------------------------------------------------\n" );
    
    const int ServerPort = 30000;
    const int ClientPort = 30001;
    const int ProtocolId = 0x11112222;
    const float DeltaTime = 0.001f;
    const float TimeOut = 0.1f;
    
    ReliableConnection client( ProtocolId, TimeOut );
    ReliableConnection server( ProtocolId, TimeOut );
    
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

void test_reliable_connection_acks()
{
    printf( "-----------------------------------------------------\n" );
    printf( "test reliable connection acks\n" );
    printf( "-----------------------------------------------------\n" );
    
    const int ServerPort = 30000;
    const int ClientPort = 30001;
    const int ProtocolId = 0x11112222;
    const float DeltaTime = 0.001f;
    const float TimeOut = 0.1f;
    const unsigned int PacketCount = 100;
    
    ReliableConnection client( ProtocolId, TimeOut );
    ReliableConnection server( ProtocolId, TimeOut );
    
    check( client.Start( ClientPort ) );
    check( server.Start( ServerPort ) );
    
    client.Connect( Address(127,0,0,1,ServerPort ) );
    server.Listen();
    
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
        if ( !client.IsConnecting() && client.ConnectFailed() )
            break;
        
        if ( allPacketsAcked )
            break;
        
        unsigned char packet[256];
        for ( unsigned int i = 0; i < sizeof(packet); ++i )
            packet[i] = (unsigned char) i;
        
        server.SendPacket( packet, sizeof(packet) );
        client.SendPacket( packet, sizeof(packet) );
        
        while ( true )
        {
            unsigned char packet[256];
            int bytes_read = client.ReceivePacket( packet, sizeof(packet) );
            if ( bytes_read == 0 )
                break;
            printf("client received %i bytes from server\n", bytes_read);
            check( bytes_read == sizeof(packet) );
            for ( unsigned int i = 0; i < sizeof(packet); ++i )
                check( packet[i] == (unsigned char) i );
        }
        
        while ( true )
        {
            unsigned char packet[256];
            int bytes_read = server.ReceivePacket( packet, sizeof(packet) );
            if ( bytes_read == 0 )
                break;
            printf("server received %i bytes from client\n", bytes_read);
            check( bytes_read == sizeof(packet) );
            for ( unsigned int i = 0; i < sizeof(packet); ++i )
                check( packet[i] == (unsigned char) i );
        }
        
        int ack_count = 0;
        unsigned int * acks = NULL;
        client.GetReliabilitySystem().GetAcks( &acks, ack_count );
        printf("client seq: %i, remote seq: %i\n",
               client.GetReliabilitySystem().GetLocalSequence(),
               client.GetReliabilitySystem().GetLocalSequence());

        check( ack_count == 0 || ack_count != 0 && acks );
        printf("client has %i acks from server\n", ack_count);

        for ( int i = 0; i < ack_count; ++i )
        {
            unsigned int ack = acks[i];
            printf("current ack: %i/%i\n", ack, ack_count);
            if ( ack < PacketCount )
            {
                check( clientAckedPackets[ack] == false );
                clientAckedPackets[ack] = true;
            }
        }
        
        server.GetReliabilitySystem().GetAcks( &acks, ack_count );
        printf("server seq: %i, remote seq: %i\n",
               server.GetReliabilitySystem().GetLocalSequence(),
               server.GetReliabilitySystem().GetLocalSequence());

        check( ack_count == 0 || ack_count != 0 && acks );
        printf("server has %i acks from client\n", ack_count);
        for ( int i = 0; i < ack_count; ++i )
        {
            unsigned int ack = acks[i];
            printf("current ack: %i/%i\n", ack, ack_count);

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
        
        client.Update( DeltaTime );
        server.Update( DeltaTime );
    }
    
    check( client.IsConnected() );
    check( server.IsConnected() );
}

void test_reliable_connection_ack_bits()
{
    printf( "-----------------------------------------------------\n" );
    printf( "test reliable connection ack bits\n" );
    printf( "-----------------------------------------------------\n" );
    
    const int ServerPort = 30000;
    const int ClientPort = 30001;
    const int ProtocolId = 0x11112222;
    const float DeltaTime = 0.001f;
    const float TimeOut = 0.1f;
    const unsigned int PacketCount = 100;
    
    ReliableConnection client( ProtocolId, TimeOut );
    ReliableConnection server( ProtocolId, TimeOut );
    
    check( client.Start( ClientPort ) );
    check( server.Start( ServerPort ) );
    
    client.Connect( Address(127,0,0,1,ServerPort ) );
    server.Listen();
    
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
        if ( !client.IsConnecting() && client.ConnectFailed() )
            break;
        
        if ( allPacketsAcked )
            break;
        
        unsigned char packet[256];
        for ( unsigned int i = 0; i < sizeof(packet); ++i )
            packet[i] = (unsigned char) i;
        
        for ( int i = 0; i < 10; ++i )
        {
            client.SendPacket( packet, sizeof(packet) );
            
            while ( true )
            {
                unsigned char packet[256];
                int bytes_read = client.ReceivePacket( packet, sizeof(packet) );
                if ( bytes_read == 0 )
                    break;
                check( bytes_read == sizeof(packet) );
                for ( unsigned int i = 0; i < sizeof(packet); ++i )
                    check( packet[i] == (unsigned char) i );
            }
            
            int ack_count = 0;
            unsigned int * acks = NULL;
            client.GetReliabilitySystem().GetAcks( &acks, ack_count );
            check( ack_count == 0 || ack_count != 0 && acks );
            for ( int i = 0; i < ack_count; ++i )
            {
                unsigned int ack = acks[i];
                if ( ack < PacketCount )
                {
                    check( !clientAckedPackets[ack] );
                    clientAckedPackets[ack] = true;
                }
            }
            
            client.Update( DeltaTime * 0.1f );
        }
        
        server.SendPacket( packet, sizeof(packet) );
        
        while ( true )
        {
            unsigned char packet[256];
            int bytes_read = server.ReceivePacket( packet, sizeof(packet) );
            if ( bytes_read == 0 )
                break;
            check( bytes_read == sizeof(packet) );
            for ( unsigned int i = 0; i < sizeof(packet); ++i )
                check( packet[i] == (unsigned char) i );
        }
        
        int ack_count = 0;
        unsigned int * acks = NULL;
        server.GetReliabilitySystem().GetAcks( &acks, ack_count );
        check( ack_count == 0 || ack_count != 0 && acks );
        for ( int i = 0; i < ack_count; ++i )
        {
            unsigned int ack = acks[i];
            if ( ack < PacketCount )
            {
                check( !serverAckedPackets[ack] );
                serverAckedPackets[ack] = true;
            }
        }
        
        unsigned int clientAckCount = 0;
        unsigned int serverAckCount = 0;
        for ( unsigned int i = 0; i < PacketCount; ++i )
        {
            if ( clientAckedPackets[i] )
                clientAckCount++;
            if ( serverAckedPackets[i] )
                serverAckCount++;
        }
        //		printf( "client ack count = %d, server ack count = %d\n", clientAckCount, serverAckCount );
        allPacketsAcked = clientAckCount == PacketCount && serverAckCount == PacketCount;
        
        server.Update( DeltaTime );
    }
    
    check( client.IsConnected() );
    check( server.IsConnected() );
}


class PacketLossReliableConnection : public ReliableConnection
{
public:
    // unit test controls
    PacketLossReliableConnection(unsigned int protocolId,
                                 float timeout,
                                 unsigned int max_sequence = 0xFFFFFFFF) :
    ReliableConnection( protocolId, timeout, max_sequence )
    {
        packet_loss_mask = 0;
    }
    void SetPacketLossMask( unsigned int mask ) { packet_loss_mask = mask; }
    bool SendPacket( const unsigned char data[], int size )
    {
        if ( GetReliabilitySystem().GetLocalSequence() & packet_loss_mask )
        {
            GetReliabilitySystem().PacketSent( size );
            return true;
        }
        return ReliableConnection::SendPacket(data, size);
    }
    void Update( float deltaTime ) {
        ReliableConnection::Update(deltaTime);
        GetReliabilitySystem().Validate();
    }
    
private:
    unsigned int packet_loss_mask;			// mask sequence number, if non-zero, drop packet - for unit test only
};

void test_reliable_connection_packet_loss()
{
    printf( "-----------------------------------------------------\n" );
    printf( "test reliable connection packet loss\n" );
    printf( "-----------------------------------------------------\n" );
    
    const int ServerPort = 30000;
    const int ClientPort = 30001;
    const int ProtocolId = 0x11112222;
    const float DeltaTime = 0.001f;
    const float TimeOut = 0.1f;
    const unsigned int PacketCount = 100;
    
    PacketLossReliableConnection client( ProtocolId, TimeOut );
    PacketLossReliableConnection server( ProtocolId, TimeOut );
    
    client.SetPacketLossMask( 1 );
    server.SetPacketLossMask( 1 );
    
    check( client.Start( ClientPort ) );
    check( server.Start( ServerPort ) );
    
    client.Connect( Address(127,0,0,1,ServerPort ) );
    server.Listen();
    
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
        if ( !client.IsConnecting() && client.ConnectFailed() )
            break;
        
        if ( allPacketsAcked )
            break;
        
        unsigned char packet[256];
        for ( unsigned int i = 0; i < sizeof(packet); ++i )
            packet[i] = (unsigned char) i;
        
        for ( int i = 0; i < 10; ++i )
        {
            client.SendPacket( packet, sizeof(packet) );
            
            while ( true )
            {
                unsigned char packet[256];
                int bytes_read = client.ReceivePacket( packet, sizeof(packet) );
                if ( bytes_read == 0 )
                    break;
                check( bytes_read == sizeof(packet) );
                for ( unsigned int i = 0; i < sizeof(packet); ++i )
                    check( packet[i] == (unsigned char) i );
            }
            
            int ack_count = 0;
            unsigned int * acks = NULL;
            client.GetReliabilitySystem().GetAcks( &acks, ack_count );
            check( ack_count == 0 || ack_count != 0 && acks );
            for ( int i = 0; i < ack_count; ++i )
            {
                unsigned int ack = acks[i];
                if ( ack < PacketCount )
                {
                    check( !clientAckedPackets[ack] );
                    check ( ( ack & 1 ) == 0 );
                    clientAckedPackets[ack] = true;
                }
            }
            
            client.Update( DeltaTime * 0.1f );
        }
        
        server.SendPacket( packet, sizeof(packet) );
        
        while ( true )
        {
            unsigned char packet[256];
            int bytes_read = server.ReceivePacket( packet, sizeof(packet) );
            if ( bytes_read == 0 )
                break;
            check( bytes_read == sizeof(packet) );
            for ( unsigned int i = 0; i < sizeof(packet); ++i )
                check( packet[i] == (unsigned char) i );
        }
        
        int ack_count = 0;
        unsigned int * acks = NULL;
        server.GetReliabilitySystem().GetAcks( &acks, ack_count );
        check( ack_count == 0 || ack_count != 0 && acks );
        for ( int i = 0; i < ack_count; ++i )
        {
            unsigned int ack = acks[i];
            if ( ack < PacketCount )
            {
                check( !serverAckedPackets[ack] );
                check( ( ack & 1 ) == 0 );
                serverAckedPackets[ack] = true;
            }
        }
        
        unsigned int clientAckCount = 0;
        unsigned int serverAckCount = 0;
        for ( unsigned int i = 0; i < PacketCount; ++i )
        {
            if ( ( i & 1 ) != 0 )
            {
                check( clientAckedPackets[i] == false );
                check( serverAckedPackets[i] == false );
            }
            if ( clientAckedPackets[i] )
                clientAckCount++;
            if ( serverAckedPackets[i] )
                serverAckCount++;
        }
        allPacketsAcked = clientAckCount == PacketCount / 2 && serverAckCount == PacketCount / 2;
        
        server.Update( DeltaTime );
    }
    
    check( client.IsConnected() );
    check( server.IsConnected() );
}

void test_reliable_connection_sequence_wrap_around()
{
    printf( "-----------------------------------------------------\n" );
    printf( "test reliable connection sequence wrap around\n" );
    printf( "-----------------------------------------------------\n" );
    
    const int ServerPort = 30000;
    const int ClientPort = 30001;
    const int ProtocolId = 0x11112222;
    const float DeltaTime = 0.05f;
    const float TimeOut = 1000.0f;
    const unsigned int PacketCount = 256;
    const unsigned int MaxSequence = 31;		// [0,31]
    
    ReliableConnection client( ProtocolId, TimeOut, MaxSequence );
    ReliableConnection server( ProtocolId, TimeOut, MaxSequence );
    
    check( client.Start( ClientPort ) );
    check( server.Start( ServerPort ) );
    
    client.Connect( Address(127,0,0,1,ServerPort ) );
    server.Listen();
    
    unsigned int clientAckCount[MaxSequence+1];
    unsigned int serverAckCount[MaxSequence+1];
    for ( unsigned int i = 0; i <= MaxSequence; ++i )
    {
        clientAckCount[i] = 0;
        serverAckCount[i] = 0;
    }
    
    bool allPacketsAcked = false;
    
    while ( true )
    {
        if ( !client.IsConnecting() && client.ConnectFailed() )
            break;
        
        if ( allPacketsAcked )
            break;
        
        unsigned char packet[256];
        for ( unsigned int i = 0; i < sizeof(packet); ++i )
            packet[i] = (unsigned char) i;
        
        server.SendPacket( packet, sizeof(packet) );
        client.SendPacket( packet, sizeof(packet) );
        
        while ( true )
        {
            unsigned char packet[256];
            int bytes_read = client.ReceivePacket( packet, sizeof(packet) );
            if ( bytes_read == 0 )
                break;
            check( bytes_read == sizeof(packet) );
            for ( unsigned int i = 0; i < sizeof(packet); ++i )
                check( packet[i] == (unsigned char) i );
        }
        
        while ( true )
        {
            unsigned char packet[256];
            int bytes_read = server.ReceivePacket( packet, sizeof(packet) );
            if ( bytes_read == 0 )
                break;
            check( bytes_read == sizeof(packet) );
            for ( unsigned int i = 0; i < sizeof(packet); ++i )
                check( packet[i] == (unsigned char) i );
        }
        
        int ack_count = 0;
        unsigned int * acks = NULL;
        client.GetReliabilitySystem().GetAcks( &acks, ack_count );
        check( ack_count == 0 || ack_count != 0 && acks );
        for ( int i = 0; i < ack_count; ++i )
        {
            unsigned int ack = acks[i];
            check( ack <= MaxSequence );
            clientAckCount[ack] += 1;
        }
        
        server.GetReliabilitySystem().GetAcks( &acks, ack_count );
        check( ack_count == 0 || ack_count != 0 && acks );
        for ( int i = 0; i < ack_count; ++i )
        {
            unsigned int ack = acks[i];
            check( ack <= MaxSequence );
            serverAckCount[ack]++;
        }
        
        unsigned int totalClientAcks = 0;
        unsigned int totalServerAcks = 0;
        for ( unsigned int i = 0; i <= MaxSequence; ++i )
        {
            totalClientAcks += clientAckCount[i];
            totalServerAcks += serverAckCount[i];
        }
        allPacketsAcked = totalClientAcks >= PacketCount && totalServerAcks >= PacketCount;
        
        // note: test above is not very specific, we can do better...
        
        client.Update( DeltaTime );
        server.Update( DeltaTime );
    }
    
    check( client.IsConnected() );
    check( server.IsConnected() );
}

void RunReliabilityTests()
{
    printf( "-----------------------------------------------------\n" );
    printf( "running reliable connection tests...\n" );

    test_packet_queue();
    test_reliability_system();
    
    test_reliable_connection_join();
    test_reliable_connection_join_timeout();
    test_reliable_connection_join_busy();
    test_reliable_connection_rejoin();
    test_reliable_connection_payload();
    test_reliable_connection_acks();
    test_reliable_connection_ack_bits();
    test_reliable_connection_packet_loss();
    test_reliable_connection_sequence_wrap_around();

    printf( "-----------------------------------------------------\n" );
    printf( "reliable connection tests passed!\n" );
}
