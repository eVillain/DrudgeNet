#include "Node.h"
#include "Serialization.h"
#include <cassert>

namespace Net
{
    Node::Node(unsigned int protocolId,
               float sendRate,
               float timeout,
               int maxPacketSize)
    {
        this->protocolId = protocolId;
        this->sendRate = sendRate;
        this->timeout = timeout;
        this->maxPacketSize = maxPacketSize;
        state = Disconnected;
        running = false;
        ClearData();
    }
    
    Node::~Node()
    {
        if ( running )
            Stop();
    }
    
    bool Node::Start( int port )
    {
        assert( !running );
        printf( "Node: starting node on port %d\n", port );
        if ( !socket.Open( port ) )
            return false;
        running = true;
        return true;
    }
    
    void Node::Stop()
    {
        assert( running );
        printf( "Node: stopping node\n" );
        ClearData();
        socket.Close();
        running = false;
    }
    
    void Node::Join( const Address & address )
    {
        printf( "Node: joining mesh at %d.%d.%d.%d:%d\n",
               address.GetA(),
               address.GetB(),
               address.GetC(),
               address.GetD(),
               address.GetPort());
        ClearData();
        state = Joining;
        meshAddress = address;
    }
    
    void Node::Update( float deltaTime )
    {
        assert( running );
        ReceivePackets();
        SendPackets( deltaTime );
        CheckForTimeout( deltaTime );
    }
    
    bool Node::IsNodeConnected( int nodeId )
    {
        assert( nodeId >= 0 );
        assert( nodeId < (int) nodes.size() );
        return nodes[nodeId].connected;
    }
    
    Address Node::GetNodeAddress( int nodeId )
    {
        assert( nodeId >= 0 );
        assert( nodeId < (int) nodes.size() );
        return nodes[nodeId].address;
    }
    
    int Node::GetMaxNodes() const
    {
        assert( nodes.size() <= 255 );
        return (int) nodes.size();
    }
    
    bool Node::SendPacket( int nodeId, const unsigned char data[], int size )
    {
        assert( running );
        if ( nodes.size() == 0 )
            return false;	// not connected yet
        assert( nodeId >= 0 );
        assert( nodeId < (int) nodes.size() );
        if ( nodeId < 0 || nodeId >= (int) nodes.size() )
            return false;
        if ( !nodes[nodeId].connected )
            return false;
        assert( size <= maxPacketSize );
        if ( size > maxPacketSize )
            return false;
        return socket.Send( nodes[nodeId].address, data, size );
    }
    
    int Node::ReceivePacket( int & nodeId, unsigned char data[], int size )
    {
        assert( running );
        if ( !receivedPackets.empty() )
        {
            BufferedPacket * packet = receivedPackets.top();
            assert( packet );
            if ( (int) packet->data.size() <= size )
            {
                nodeId = packet->nodeId;
                size = (int)packet->data.size();
                memcpy( data, &packet->data[0], size );
                delete packet;
                receivedPackets.pop();
                return size;
            }
            else
            {
                delete packet;
                receivedPackets.pop();
            }
        }
        return 0;
    }
     
    void Node::ReceivePackets()
    {
        while ( true )
        {
            Address sender;
            unsigned char *data;
            data = new unsigned char[maxPacketSize];
            int size = socket.Receive( sender, data, maxPacketSize );
            if ( !size )
                break;
//            printf("Node %i: received %i bytes\n", localNodeId, size);
            ProcessPacket( sender, data, size );
        }
    }
    
    void Node::ProcessPacket( const Address & sender, unsigned char data[], int size )
    {
        assert( sender != Address() );
        assert( size > 0 );
        assert( data );
        // is packet from the mesh?
        if ( sender == meshAddress )
        {
//            printf("Node %i: received %i bytes from mesh\n", localNodeId, size);

            // *** packet sent from the mesh ***
            // ignore packets that dont have the correct protocol id
            unsigned int firstIntegerInPacket;
            Serialization::ReadInteger(data, firstIntegerInPacket);
            if ( firstIntegerInPacket != protocolId )
                return;
            // determine packet type
            enum PacketType { ConnectionAccepted, Update };
            PacketType packetType;
            if ( data[4] == 0 )
                packetType = ConnectionAccepted;
            else if ( data[4] == 1 )
                packetType = Update;
            else
                return;
            // handle packet type
            switch ( packetType )
            {
                case ConnectionAccepted:
                {
                    if ( size != 7 )
                        return;
                    if ( state == Joining )
                    {
                        localNodeId = data[5];
                        nodes.resize( data[6] );
                        printf("Node %i: joined mesh!\n", localNodeId );
                        state = Joined;
                    }
                    timeoutAccumulator = 0.0f;
                }
                    break;
                case Update:
                {
                    if ( size != (int) ( 5 + nodes.size() * 10 ) )
                        return;
                    if ( state == Joined )
                    {
                        // process update packet
                        unsigned char * ptr = &data[5];
                        for ( unsigned int i = 0; i < nodes.size(); ++i )
                        {
                            unsigned char a = ptr[0];
                            unsigned char b = ptr[1];
                            unsigned char c = ptr[2];
                            unsigned char d = ptr[3];
                            unsigned short port = (unsigned short)ptr[4] << 8 | (unsigned short)ptr[5];
                            int nodeID;
                            Serialization::ReadInteger(&ptr[6], (unsigned int&)nodeID);
                            if ((nodeID == 0 && localNodeId != 0) &&
                                (a == 127 && b == 0 && c == 0 && d == 1)) {
                                // Localhost address, it's from the mesh localnode and needs to be fixed
                                a = meshAddress.GetA();
                                b = meshAddress.GetB();
                                c = meshAddress.GetC();
                                d = meshAddress.GetD();
                            }
                            Address address( a, b, c, d, port );
                            if ( address.GetAddress() != 0 )
                            {
                                // node is connected
                                if ( address != nodes[i].address )
                                {
                                    nodes[i].connected = true;
                                    nodes[i].address = address;
                                    nodes[i].nodeID = nodeID;
                                    addr2node[address] = &nodes[i];
                                    printf("Node %i: node %i @%d.%d.%d.%d:%i connected\n",
                                           localNodeId,
                                           nodes[i].nodeID,
                                           nodes[i].address.GetA(),
                                           nodes[i].address.GetB(),
                                           nodes[i].address.GetC(),
                                           nodes[i].address.GetD(),
                                           nodes[i].address.GetPort());
                                }
                            }
                            else
                            {
                                // node is not connected
                                if ( nodes[i].connected )
                                {
                                    printf( "Node %i: node %i @%d.%d.%d.%d:%i disconnected\n",
                                           localNodeId,
                                           nodes[i].nodeID,
                                           nodes[i].address.GetA(),
                                           nodes[i].address.GetB(),
                                           nodes[i].address.GetC(),
                                           nodes[i].address.GetD(),
                                           nodes[i].address.GetPort());
                                    AddrToNode::iterator itor = addr2node.find( nodes[i].address );
                                    assert( itor != addr2node.end() );
                                    addr2node.erase( itor );
                                    nodes[i].connected = false;
                                    nodes[i].address = Address();
                                }
                            }
                            ptr += 10;
                        }
                    }
                    timeoutAccumulator = 0.0f;
                }
                    break;
            }
        }
        else
        {
            AddrToNode::iterator itor = addr2node.find( sender );
            if ( itor != addr2node.end() )
            {
                // *** packet sent from another node ***
                NodeState * node = itor->second;
                assert( node );
                int nodeId = (int) ( node - &nodes[0] );
//                printf("Node %i: received package from node %i, size %i\n", localNodeId, nodeId, size);
                assert( nodeId >= 0 );
                assert( nodeId < (int) nodes.size() );
                BufferedPacket * packet = new BufferedPacket;
                packet->nodeId = nodeId;
                packet->data.resize( size );
                memcpy( &packet->data[0], data, size );
                receivedPackets.push( packet );
            }
        }
    }
    
    void Node::SendPackets( float deltaTime )
    {
        sendAccumulator += deltaTime;
        while ( sendAccumulator > sendRate )
        {
            if ( state == Joining )
            {
                // node is joining: send "join request" packets
                unsigned char packet[5];
                Serialization::WriteInteger(packet, protocolId);
                packet[4] = 0;
                socket.Send( meshAddress, packet, sizeof(packet) );
            }
            else if ( state == Joined )
            {
                // node is joined: send "keep alive" packets
                unsigned char packet[5];
                Serialization::WriteInteger(packet, protocolId);
                packet[4] = 1;
                socket.Send( meshAddress, packet, sizeof(packet) );
            }
            sendAccumulator -= sendRate;
        }
    }
    
    void Node::CheckForTimeout( float deltaTime )
    {
        if ( state == Joining || state == Joined )
        {
            timeoutAccumulator += deltaTime;
            if ( timeoutAccumulator > timeout )
            {
                if ( state == Joining )
                {
                    printf( "Node: join failed\n" );
                    state = JoinFail;
                }
                else
                {
                    printf( "Node %i: node timed out\n", localNodeId );
                    state = Disconnected;
                }
                ClearData();
            }
        }
    }
    
    void Node::ClearData()
    {
        nodes.clear();
        addr2node.clear();
        while ( !receivedPackets.empty() )
        {
            BufferedPacket * packet = receivedPackets.top();
            delete packet;
            receivedPackets.pop();
        }
        sendAccumulator = 0.0f;
        timeoutAccumulator = 0.0f;
        localNodeId = -1;
        meshAddress = Address();
    }
}
