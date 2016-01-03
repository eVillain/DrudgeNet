#ifndef NET_RELIABLE_CONNECTION_H
#define NET_RELIABLE_CONNECTION_H

#include "Connection.h"
#include "ReliabilitySystem.h"

// connection with reliability (seq/ack)

namespace Net
{
    class ReliableConnection : public Connection
    {
    public:
        
        ReliableConnection( unsigned int protocolId, float timeout, unsigned int max_sequence = 0xFFFFFFFF );
        
        ~ReliableConnection();
        
        // overriden functions from "Connection"
        
        bool SendPacket( const unsigned char data[], int size );
        
        int ReceivePacket( unsigned char data[], int size );
        
        void Update( float deltaTime );
        
        int GetHeaderSize() const;
        
        ReliabilitySystem & GetReliabilitySystem() { return reliabilitySystem; }

    protected:
        
        void WriteInteger( unsigned char * data, unsigned int value );
        
        void WriteHeader( unsigned char * header, unsigned int sequence, unsigned int ack, unsigned int ack_bits );
        
        void ReadInteger( const unsigned char * data, unsigned int & value );
        
        void ReadHeader( const unsigned char * header, unsigned int & sequence, unsigned int & ack, unsigned int & ack_bits );
        
        virtual void OnStop();
        
        virtual void OnDisconnect();
        
    private:
        
        void ClearData();
        
        ReliabilitySystem reliabilitySystem;	// reliability system: manages sequence numbers and acks, tracks network stats etc.
    };

}

#endif /* ReliableConnection_h */
