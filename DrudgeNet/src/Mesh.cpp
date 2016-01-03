#include "Mesh.h"
#include "Serialization.h"
#include <cassert>

namespace Net
{
    Mesh::Mesh(unsigned int protocolId,
               int maxNodes,
               float sendRate,
               float timeout)
    {
        assert( maxNodes >= 1 );
        assert( maxNodes <= 255 );
        this->protocolId = protocolId;
        this->sendRate = sendRate;
        this->timeout = timeout;
        nodes.resize( maxNodes );
        running = false;
        sendAccumulator = 0.0f;
    }
    
    Mesh::~Mesh()
    {
        if ( running )
            Stop();
    }
    
    bool Mesh::Start( int port )
    {
        assert( !running );
        printf( "Mesh: starting mesh on port %d\n", port );
        if ( !socket.Open( port ) )
            return false;
        running = true;
        return true;
    }
    
    void Mesh::Stop()
    {
        assert( running );
        printf( "Mesh: stopping mesh\n" );
        socket.Close();
        id2node.clear();
        addr2node.clear();
        for ( unsigned int i = 0; i < nodes.size(); ++i )
            nodes[i] = NodeState();
        running = false;
        sendAccumulator = 0.0f;
    }
    
    void Mesh::Update( float deltaTime )
    {
        assert( running );
        ReceivePackets();
        SendPackets( deltaTime );
        CheckForTimeouts( deltaTime );
    }
    
    bool Mesh::IsNodeConnected( int nodeId )
    {
        assert( nodeId >= 0 );
        assert( nodeId < (int) nodes.size() );
        return nodes[nodeId].mode == NodeState::Connected;
    }
    
    Address Mesh::GetNodeAddress( int nodeId )
    {
        assert( nodeId >= 0 );
        assert( nodeId < (int) nodes.size() );
        return nodes[nodeId].address;
    }
    
    int Mesh::GetMaxAllowedNodes() const
    {
        assert( nodes.size() <= 255 );
        return (int) nodes.size();
    }
    
    void Mesh::Reserve( int nodeId, const Address & address )
    {
        assert( nodeId >= 0 );
        assert( nodeId < (int) nodes.size() );
        printf( "Mesh: reserving node id %d for %d.%d.%d.%d:%d\n",
               nodeId, address.GetA(), address.GetB(), address.GetC(), address.GetD(), address.GetPort() );
        nodes[nodeId].mode = NodeState::ConnectionAccept;
        nodes[nodeId].nodeId = nodeId;
        nodes[nodeId].address = address;
        addr2node.insert( std::make_pair( address, &nodes[nodeId] ) );
    }
    
    void Mesh::ReceivePackets()
    {
        while ( true )
        {
            Address sender;
            unsigned char data[256];
            int size = socket.Receive( sender, data, sizeof(data) );
            if ( !size )
                break;
            ProcessPacket( sender, data, size );
        }
    }
    
    void Mesh::ProcessPacket( const Address & sender, unsigned char data[], int size )
    {
        assert( sender != Address() );
        assert( size > 0 );
        assert( data );
        // ignore packets that dont have the correct protocol id
        unsigned int firstIntegerInPacket;
        Serialization::ReadInteger(data, firstIntegerInPacket);
        //( unsigned(data[0]) << 24 ) | ( unsigned(data[1]) << 16 ) |
        //( unsigned(data[2]) << 8 )  | unsigned(data[3]);
        if ( firstIntegerInPacket != protocolId )
            return;
        // determine packet type
        enum PacketType { JoinRequest, KeepAlive };
        PacketType packetType;
        if ( data[4] == 0 )
            packetType = JoinRequest;
        else if ( data[4] == 1 )
            packetType = KeepAlive;
        else
            return;
        // process packet type
        switch ( packetType )
        {
            case JoinRequest:
            {
                // is address already joining or joined?
                AddrToNode::iterator itor = addr2node.find( sender );
                if ( itor == addr2node.end() )
                {
                    // no entry for address, start join process...
                    int freeSlot = -1;
                    for ( unsigned int i = 0; i < nodes.size(); ++i )
                    {
                        if ( nodes[i].mode == NodeState::Disconnected )
                        {
                            freeSlot = (int) i;
                            break;
                        }
                    }
                    if ( freeSlot >= 0 )
                    {
                        printf( "Mesh: accepting %d.%d.%d.%d:%d as node %d\n",
                               sender.GetA(), sender.GetB(), sender.GetC(), sender.GetD(), sender.GetPort(), freeSlot );
                        assert( nodes[freeSlot].mode == NodeState::Disconnected );
                        nodes[freeSlot].mode = NodeState::ConnectionAccept;
                        nodes[freeSlot].nodeId = freeSlot;
                        nodes[freeSlot].address = sender;
                        addr2node.insert( std::make_pair( sender, &nodes[freeSlot] ) );
                    }
                }
                else if ( itor->second->mode == NodeState::ConnectionAccept )
                {
                    // reset timeout accumulator, but only while joining
                    itor->second->timeoutAccumulator = 0.0f;
                }
            }
                break;
            case KeepAlive:
            {
                AddrToNode::iterator itor = addr2node.find( sender );
                if ( itor != addr2node.end() )
                {
                    // progress from "connection accept" to "connected"
                    if ( itor->second->mode == NodeState::ConnectionAccept )
                    {
                        itor->second->mode = NodeState::Connected;
                        printf( "Mesh: completing join of node %d\n", itor->second->nodeId );
                    }
                    // reset timeout accumulator for node
                    itor->second->timeoutAccumulator = 0.0f;
                }
            }
                break;
        }
    }
    
    void Mesh::SendPackets( float deltaTime )
    {
        sendAccumulator += deltaTime;
        while ( sendAccumulator > sendRate )
        {
            for ( unsigned int i = 0; i < nodes.size(); ++i )
            {
                if ( nodes[i].mode == NodeState::ConnectionAccept )
                {
                    // node is negotiating join: send "connection accepted" packets
                    unsigned char packet[7];
                    Serialization::WriteInteger(packet, protocolId);
                    packet[4] = 0;
                    packet[5] = (unsigned char) i;
                    packet[6] = (unsigned char) nodes.size();
                    socket.Send( nodes[i].address, packet, sizeof(packet) );
                }
                else if ( nodes[i].mode == NodeState::Connected )
                {
                    // node is connected: send "update" packets
                    unsigned char *packet;
                    int packetSize = (int)(5+10*nodes.size());
                    packet = new unsigned char[packetSize];
                    Serialization::WriteInteger(packet, protocolId);
                    packet[4] = 1;
                    unsigned char * ptr = &packet[5];
                    for ( unsigned int j = 0; j < nodes.size(); ++j )
                    {
                        ptr[0] = nodes[j].address.GetA();
                        ptr[1] = nodes[j].address.GetB();
                        ptr[2] = nodes[j].address.GetC();
                        ptr[3] = nodes[j].address.GetD();
                        ptr[4] = (unsigned char) ( ( nodes[j].address.GetPort() >> 8 ) & 0xFF );
                        ptr[5] = (unsigned char) ( ( nodes[j].address.GetPort() ) & 0xFF );
                        ptr[6] = (unsigned char) ( ( nodes[j].nodeId >> 24 ) & 0xFF );
                        ptr[7] = (unsigned char) ( ( nodes[j].nodeId >> 16 ) & 0xFF );
                        ptr[8] = (unsigned char) ( ( nodes[j].nodeId >> 8 ) & 0xFF );
                        ptr[9] = (unsigned char) ( ( nodes[j].nodeId ) & 0xFF );
                        ptr += 10;
                    }
                    socket.Send( nodes[i].address, packet, packetSize );
                }
            }
            sendAccumulator -= sendRate;
        }
    }
    
    void Mesh::CheckForTimeouts( float deltaTime )
    {
        for ( unsigned int i = 0; i < nodes.size(); ++i )
        {
            if ( nodes[i].mode != NodeState::Disconnected )
            {
                nodes[i].timeoutAccumulator += deltaTime;
                if ( nodes[i].timeoutAccumulator > timeout )
                {
                    printf( "Mesh: node %d timed out\n", nodes[i].nodeId );
                    AddrToNode::iterator addr_itor = addr2node.find( nodes[i].address );
                    assert( addr_itor != addr2node.end() );
                    addr2node.erase( addr_itor );
                    nodes[i] = NodeState();
                }
            }
        }
    }
    
}
