#ifndef NET_TRANSPORT_LAN_H
#define NET_TRANSPORT_LAN_H

#include "Transport.h"
#include <vector>
#include <map>

namespace Net
{
    // lan transport implementation
    //  + servers are advertised via net beacon
    //  + lan lobby is filled via net listener
    //  + a mesh runs on the server IP and manages node connections
    //  + a node runs on each transport, including a local node on the server with the mesh
    
    class TransportLAN : public Transport
    {
    public:
        
        // static interface
        
        static bool Initialize();
        
        static void Shutdown();
        
        static bool GetHostName( char hostname[], int size );
        
        static void UnitTest();
        
        // lan specific interface
        
        TransportLAN();
        ~TransportLAN();
        
        struct Config
        {
            unsigned short meshPort;
            unsigned short serverPort;
            unsigned short clientPort;
            unsigned short beaconPort;
            unsigned short listenerPort;
            unsigned int protocolId;
            float meshSendRate;
            float timeout;
            int maxNodes;
            
            Config()
            {
                meshPort = 30000;
                clientPort = 30001;
                serverPort = 30002;
                beaconPort = 40000;
                listenerPort = 40001;
                protocolId = 0x12345678;
                meshSendRate = 0.25f;
                timeout = 10.0f;
                maxNodes = 4;
            }
        };
        
        void Configure( Config & config );
        
        const Config & GetConfig() const;
        
        bool StartServer( const char name[] );
        
        bool ConnectClient( const char server[] );
        
        bool IsConnected() const;
        
        bool ConnectFailed() const;
        
        bool EnterLobby();
        
        int GetLobbyEntryCount();
        
        struct LobbyEntry
        {
            char name[65];
            char address[65];
        };
        
        bool GetLobbyEntryAtIndex(int index,
                                  LobbyEntry & entry);
        
        void Stop();
        
        // implement transport interface
        
        bool IsNodeConnected( int nodeId );
        
        int GetLocalNodeId() const;
        
        int GetMaxNodes() const;
        
        bool SendPacket(int nodeId,
                        const unsigned char data[],
                        int size);
        
        int ReceivePacket(int & nodeId,
                          unsigned char data[],
                          int size);
        
        class ReliabilitySystem & GetReliability( int nodeId );
        
        void Update( float deltaTime );
        
        TransportType GetType() const;
        
    private:
        void WriteHeader(unsigned char * header,
                         unsigned int sequence,
                         unsigned int ack,
                         unsigned int ack_bits);
        
        void ReadHeader(const unsigned char * header,
                        unsigned int & sequence,
                        unsigned int & ack,
                        unsigned int & ack_bits);
        
        Config config;
        class Mesh * mesh;
        class Node * node;
        class Beacon * beacon;
        class Listener * listener;
        float beaconAccumulator;
        
        bool connectingByName;
        char connectName[65];
        float connectAccumulator;
        bool connectFailed;
        
        std::vector<ReliabilitySystem> reliabilitySystems;
        typedef std::map<int,ReliabilitySystem*> IdToReliability;
        IdToReliability id2reliability;
    };
}

#endif /* TransportLAN_h */
