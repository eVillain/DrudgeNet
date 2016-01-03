#ifndef NET_MESH_H
#define NET_MESH_H

#include "Socket.h"
#include <vector>
#include <map>

namespace Net
{
    class Mesh
    {
        struct NodeState
        {
            enum Mode { Disconnected, ConnectionAccept, Connected };
            Mode mode;
            float timeoutAccumulator;
            Address address;
            int nodeId;
            NodeState()
            {
                mode = Disconnected;
                address = Address();
                nodeId = -1;
                timeoutAccumulator = 0.0f;
            }
        };

    public:
        
        Mesh(unsigned int protocolId,
             int maxNodes = 255,
             float sendRate = 0.25f,
             float timeout = 10.0f);
        
        ~Mesh();
        
        bool Start( int port );
        
        void Stop();
        
        void Update( float deltaTime );
        
        bool IsNodeConnected( int nodeId );
        
        Address GetNodeAddress( int nodeId );
        
        int GetMaxAllowedNodes() const;
        
        void Reserve( int nodeId, const Address & address );
        
    protected:
        
        void ReceivePackets();
        
        void ProcessPacket( const Address & sender, unsigned char data[], int size );
        
        void SendPackets( float deltaTime );
        
        void CheckForTimeouts( float deltaTime );
        
    private:
        
        unsigned int protocolId;
        float sendRate;
        float timeout;
        
        Socket socket;
        std::vector<NodeState> nodes;
        typedef std::map<int,NodeState*> IdToNode;
        typedef std::map<Address,NodeState*> AddrToNode;
        IdToNode id2node;
        AddrToNode addr2node;
        bool running;
        float sendAccumulator;
    };
}

#endif /* Mesh_hpp */
