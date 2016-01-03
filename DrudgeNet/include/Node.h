#ifndef NET_NODE_H
#define NET_NODE_H

#include "Socket.h"

#include <stack>
#include <vector>
#include <map>

namespace Net
{
    class Node
    {
    public:
        
        Node(unsigned int protocolId,
             float sendRate = 0.25f,
             float timeout = 10.0f,
             int maxPacketSize = 1024);
        
        ~Node();
        
        bool Start( int port );
        
        void Stop();
        
        void Join( const Address & address );
        
        bool IsJoining() const { return state == Joining; }
        
        bool JoinFailed() const { return state == JoinFail; }
        
        bool IsConnected() const { return state == Joined; }
        
        int GetLocalNodeId() const { return localNodeId; }
        
        void Update( float deltaTime );
        
        bool IsNodeConnected( int nodeId );
        
        Address GetNodeAddress( int nodeId );
        
        int GetMaxNodes() const;
        
        bool SendPacket( int nodeId, const unsigned char data[], int size );
        
        int ReceivePacket( int & nodeId, unsigned char data[], int size );
        
    protected:
        
        void ReceivePackets();
        
        void ProcessPacket( const Address & sender, unsigned char data[], int size );
        
        void SendPackets( float deltaTime );
        
        void CheckForTimeout( float deltaTime );
        
        void ClearData();
        
    private:
        struct NodeState
        {
            bool connected;
            Address address;
            int nodeID;
            NodeState()
            {
                connected = false;
                address = Address();
                nodeID = -1;
            }
        };
        
        struct BufferedPacket
        {
            int nodeId;
            std::vector<unsigned char> data;
        };
        
        typedef std::stack<BufferedPacket*> PacketBuffer;
        PacketBuffer receivedPackets;
        
        unsigned int protocolId;
        float sendRate;
        float timeout;
        int maxPacketSize;
        
        Socket socket;
        std::vector<NodeState> nodes;
        typedef std::map<int,NodeState*> IdToNode;
        typedef std::map<Address,NodeState*> AddrToNode;
        AddrToNode addr2node;
        bool running;
        float sendAccumulator;
        float timeoutAccumulator;
        
        enum State
        {
            Disconnected,
            Joining,
            Joined,
            JoinFail
        };
        State state;
        Address meshAddress;
        int localNodeId;
    };
}

#endif /* Node_hpp */
