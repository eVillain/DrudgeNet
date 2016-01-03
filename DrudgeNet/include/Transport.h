#ifndef NET_TRANSPORT_H
#define NET_TRANSPORT_H

namespace Net
{
    // transport type
    
    enum TransportType
    {
        Transport_None,
        Transport_LAN,
    };
    
    // abstract network transport interface
    //  + implement this interface for different transport layers
    //  + use the reliability system classes to implement seq/ack based reliability
    
    class Transport
    {
    public:
        
        // static methods
        
        static bool Initialize( TransportType type );
        
        static void Shutdown();
        
        static Transport * Create();
        
        static void Destroy( Transport * transport );
        
        // transport interface
        
        virtual ~Transport() {};
        
        virtual bool IsNodeConnected( int nodeId ) = 0;
        
        virtual int GetLocalNodeId() const = 0;
        
        virtual int GetMaxNodes() const = 0;
        
        virtual bool SendPacket( int nodeId, const unsigned char data[], int size ) = 0;
        
        virtual int ReceivePacket( int & nodeId, unsigned char data[], int size ) = 0;
        
        virtual class ReliabilitySystem & GetReliability( int nodeId ) = 0;
        
        virtual void Update( float deltaTime ) = 0;
        
        virtual TransportType GetType() const = 0;
    };
}

#endif /* NET_TRANSPORT_H */
