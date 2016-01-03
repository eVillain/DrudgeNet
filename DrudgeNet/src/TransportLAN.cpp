#include "TransportLAN.h"

#include "Address.h"
#include "Socket.h"

#include "Beacon.h"
#include "Listener.h"

#include "Connection.h"
#include "ReliabilitySystem.h"
#include "Serialization.h"
#include "Mesh.h"
#include "Node.h"

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cassert>

#ifdef DEBUG
#define NET_UNIT_TEST
#endif


namespace Net
{
    bool TransportLAN::Initialize()
    {
        return InitializeSockets();
    }
    
    void TransportLAN::Shutdown()
    {
        return ShutdownSockets();
    }
    
    bool TransportLAN::GetHostName( char hostname[], int size )
    {
        return GetHostName( hostname, size );
    }
    
    // lan specific interface
    
    TransportLAN::TransportLAN()
    {
        mesh = nullptr;
        node = nullptr;
        beacon = nullptr;
        listener = nullptr;
        beaconAccumulator = 1.0f;
        connectingByName = false;
        connectFailed = false;
    }
    
    TransportLAN::~TransportLAN()
    {
        Stop();
    }
    
    void TransportLAN::Configure( Config & config )
    {
        // todo: assert not already running
        this->config = config;
    }
    
    const TransportLAN::Config & TransportLAN::GetConfig() const
    {
        return config;
    }
    
    bool TransportLAN::StartServer( const char name[] )
    {
        assert( !node );
        assert( !mesh );
        assert( !beacon );
        assert( !listener );
        printf( "LAN Transport: start server\n" );
        beacon = new Beacon( name, config.protocolId, config.listenerPort, config.meshPort );
        if ( !beacon->Start( config.beaconPort ) )
        {
            printf( "LAN Transport:failed to start beacon on port %d\n", config.beaconPort );
            Stop();
            return false;
        }
        mesh = new Mesh( config.protocolId, config.maxNodes, config.meshSendRate, config.timeout );
        if ( !mesh->Start( config.meshPort ) )
        {
            printf( "LAN Transport:failed to start mesh on port %d\n", config.meshPort );
            Stop();
            return 1;
        }
        node = new Node( config.protocolId, config.meshSendRate, config.timeout );
        if ( !node->Start( config.serverPort ) )
        {
            printf( "LAN Transport:failed to start node on port %d\n", config.serverPort );
            Stop();
            return 1;
        }
        mesh->Reserve( 0, Address(127,0,0,1,config.serverPort) );
        node->Join( Address(127,0,0,1,config.meshPort) );
        return true;
    }
    
    bool TransportLAN::ConnectClient( const char server[] )
    {
        assert( !node );
        assert( !mesh );
        assert( !beacon );
        assert( !listener );
        // connect by address?
        unsigned int a = 0;
        unsigned int b = 0;
        unsigned int c = 0;
        unsigned int d = 0;
        unsigned int port = 0;
        bool isAddress = false;
        if ( sscanf( server, "%d.%d.%d.%d:%d", &a, &b, &c, &d, &port ) )
        {
            isAddress = true;
        }
        else
        {
            port = config.meshPort;
            if ( sscanf( server, "%d.%d.%d.%d", &a, &b, &c, &d ) )
                isAddress = true;
        }
        // yes, connect by address
        if ( isAddress )
        {
            printf( "LAN Transport: client connect to address: %d.%d.%d.%d:%d\n", a, b, c, d, port );
            node = new Node( config.protocolId, config.meshSendRate, config.timeout );
            if ( !node->Start( config.clientPort ) )
            {
                printf( "LAN Transport: failed to start node on port %d\n", config.serverPort );
                Stop();
                return 1;
            }
            node->Join( Address( (unsigned char) a, (unsigned char) b, (unsigned char) c, (unsigned char) d, (unsigned short) port ) );
            return true;
        }
        // no, connect by hostname
        else
        {
            printf( "LAN Transport: client connect by name \"%s\"\n", server );
            listener = new Listener( config.protocolId, config.timeout );
            if ( !listener->Start( config.listenerPort ) )
            {
                printf( "LAN Transport: failed to start listener on port %d\n", config.listenerPort );
                Stop();
                return false;
            }
            connectingByName = true;
            strncpy( connectName, server, sizeof(connectName) - 1 );
            connectName[ sizeof(connectName) - 1 ] = '\0';
            connectAccumulator = 0.0f;
            connectFailed = false;
        }
        return true;
    }
    
    bool TransportLAN::IsConnected() const
    {
        return node && node->IsConnected();
    }
    
    bool TransportLAN::ConnectFailed() const
    {
        return (node && node->JoinFailed()) || (connectingByName && connectFailed);
    }
    
    bool TransportLAN::EnterLobby()
    {
        assert( !listener );
        printf( "LAN Transport: enter lobby\n" );
        listener = new Listener( config.protocolId, config.timeout );
        if ( !listener->Start( config.listenerPort ) )
        {
            printf( "LAN Transport: failed to start listener on port %d\n", config.listenerPort );
            Stop();
            return false;
        }
        return true;
    }
    
    int TransportLAN::GetLobbyEntryCount()
    {
        if ( listener )
            return listener->GetEntryCount();
        else
            return 0;
    }
    
    bool TransportLAN::GetLobbyEntryAtIndex( int index, LobbyEntry & entry )
    {
        if ( !listener || index < 0 || index >= listener->GetEntryCount() )
            return false;
        const ListenerEntry & e = listener->GetEntry( index );
        sprintf( entry.address, "%d.%d.%d.%d:%d", e.address.GetA(), e.address.GetB(), e.address.GetC(), e.address.GetD(), e.address.GetPort() );
        strncpy( entry.name, e.name, sizeof( entry.name ) );
        entry.name[ sizeof(entry.name) - 1 ] = '\0';
        return true;
    }
    
    void TransportLAN::Stop()
    {
        printf( "LAN Transport: stop\n" );
        if ( mesh )
        {
            delete mesh;
            mesh = NULL;
        }
        if ( node )
        {
            delete node;
            node = NULL;
        }
        if ( beacon )
        {
            delete beacon;
            beacon = NULL;
        }
        if ( listener )
        {
            delete listener;
            listener = NULL;
        }
        connectingByName = false;
        connectFailed = false;
    }
    
    // implement transport interface
    
    bool TransportLAN::IsNodeConnected( int nodeId )
    {
        assert( node );
        return node->IsNodeConnected( nodeId );
    }
    
    int TransportLAN::GetLocalNodeId() const
    {
        assert( node );
        return node->GetLocalNodeId();
    }
    
    int TransportLAN::GetMaxNodes() const
    {
        assert( node );
        return node->GetMaxNodes();
    }
    
    bool TransportLAN::SendPacket( int nodeId, const unsigned char data[], int size )
    {
        assert( node );
        
        ReliabilitySystem& reliabilitySystem = GetReliability(nodeId);

        const int header = 12;
        unsigned char * packet = new unsigned char[header+size];
        unsigned int seq = reliabilitySystem.GetLocalSequence();
        unsigned int ack = reliabilitySystem.GetRemoteSequence();
        unsigned int ack_bits = reliabilitySystem.GenerateAckBits();
        WriteHeader( packet, seq, ack, ack_bits );
        memcpy( packet + header, data, size );
        
        bool success = node->SendPacket( nodeId, packet, size+header );
        
        if (success) { reliabilitySystem.PacketSent( size ); }

        delete [] packet;
        
        return success;
    }
    
    int TransportLAN::ReceivePacket( int & nodeId, unsigned char data[], int size )
    {
        assert( node );
        
        const int header = 12;
        if ( size <= header )
            return false;
        unsigned char * packet = new unsigned char[header+size];
        int received_bytes = node->ReceivePacket( nodeId, packet, size + header );
        if ( received_bytes == 0 )
        {
            delete [] packet;
            return false;
        }
        if ( received_bytes <= header )
        {
            delete [] packet;
            return false;
        }
        ReliabilitySystem& reliabilitySystem = GetReliability(nodeId);

        unsigned int packet_sequence = 0;
        unsigned int packet_ack = 0;
        unsigned int packet_ack_bits = 0;
        ReadHeader( packet, packet_sequence, packet_ack, packet_ack_bits );
        reliabilitySystem.PacketReceived( packet_sequence, received_bytes - header );
        reliabilitySystem.ProcessAck( packet_ack, packet_ack_bits );
        memcpy( data, packet + header, received_bytes - header );
        delete [] packet;
        return received_bytes - header;
    }
    
    ReliabilitySystem& TransportLAN::GetReliability( int nodeId )
    {
        if (id2reliability.find(nodeId) == id2reliability.end()) {
            reliabilitySystems.resize(reliabilitySystems.size()+1);
            id2reliability[nodeId] = &reliabilitySystems[reliabilitySystems.size()-1];
        }
        return *id2reliability[nodeId];
    }
    
    void TransportLAN::Update( float deltaTime )
    {
        if ( connectingByName && !connectFailed )
        {
            assert( listener );
            const int entryCount = listener->GetEntryCount();
            for ( int i = 0; i < entryCount; ++i )
            {
                const ListenerEntry & entry = listener->GetEntry( i );
                if ( strcmp( entry.name, connectName ) == 0 )
                {
                    printf( "LAN Transport: found server %d.%d.%d.%d:%d\n",
                           entry.address.GetA(),
                           entry.address.GetB(),
                           entry.address.GetC(),
                           entry.address.GetD(),
                           entry.address.GetPort() );
                    node = new Node( config.protocolId, config.meshSendRate, config.timeout );
                    if ( !node->Start( config.clientPort ) )
                    {
                        printf( "LAN Transport: failed to start node on port %d\n", config.serverPort );
                        Stop();
                        connectFailed = true;
                        return;
                    }
                    node->Join( entry.address );
                    delete listener;
                    listener = NULL;
                    connectingByName = false;
                }
            }
            if ( connectingByName )
            {
                connectAccumulator += deltaTime;
                if ( connectAccumulator > config.timeout )
                    connectFailed = true;
            }
        }

        if ( beacon )
        {
            beaconAccumulator += deltaTime;
            while ( beaconAccumulator >= 1.0f )
            {
                beacon->Update( 1.0f );
                beaconAccumulator -= 1.0f;
            }
        }
        if ( listener )
            listener->Update( deltaTime );
        
        if ( mesh )
            mesh->Update( deltaTime );
        if ( node )
            node->Update( deltaTime );
        
        for (int i = 0; i < reliabilitySystems.size(); i++) {
            reliabilitySystems[i].Update(deltaTime);
        }
    }
    
    void TransportLAN::WriteHeader( unsigned char * header, unsigned int sequence, unsigned int ack, unsigned int ack_bits )
    {
        Serialization::WriteInteger( header, sequence );
        Serialization::WriteInteger( header + 4, ack );
        Serialization::WriteInteger( header + 8, ack_bits );
    }
    
    void TransportLAN::ReadHeader( const unsigned char * header, unsigned int & sequence, unsigned int & ack, unsigned int & ack_bits )
    {
        Serialization::ReadInteger( header, sequence );
        Serialization::ReadInteger( header + 4, ack );
        Serialization::ReadInteger( header + 8, ack_bits );
    }
    
    TransportType TransportLAN::GetType() const
    {
        return Transport_LAN;
    }
}
