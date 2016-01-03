#include "Listener.h"
#include "Serialization.h"

namespace Net
{
    Listener::Listener( unsigned int protocolId, float timeout = 10.0f ) {
        this->protocolId = protocolId;
        this->timeout = timeout;
        running = false;
        ClearData();
    }
    
    Listener::~Listener() {
        if ( running )
            Stop();
    }
    
    bool Listener::Start( int port ) {
        assert( !running );
        printf( "Listener start on port %d\n", port );
        if ( !socket.Open( port ) )
            return false;
        running = true;
        return true;
    }
    
    void Listener::Stop() {
        assert( running );
        printf( "Listener stop\n" );
        socket.Close();
        running = false;
        ClearData();
    }
    
    void Listener::Update( double deltaTime ) {
        assert( running );
        unsigned char packet[256];
        
        while ( true ) {
            Address sender;
            int bytes_read = socket.Receive( sender, packet, 256 );
            if ( bytes_read == 0 )
                break;
            if ( bytes_read < 13 )
                continue;
            unsigned int packet_zero;
            unsigned int packet_protocolId;
            unsigned int packet_ServerPort;
            unsigned char packet_stringLength;
            Serialization::ReadInteger( packet, packet_zero );
            Serialization::ReadInteger( packet + 4, packet_protocolId );
            Serialization::ReadInteger( packet + 8, packet_ServerPort );
            packet_stringLength = packet[12];
            if ( packet_zero != 0 )
                continue;
            if ( packet_protocolId != protocolId )
                continue;
            if ( packet_stringLength > 63 )
                continue;
            if ( packet_stringLength + 12 + 1 > bytes_read )
                continue;
            
            ListenerEntry entry;
            memcpy( entry.name, packet + 13, packet_stringLength );
            entry.name[packet_stringLength] = '\0';
            entry.address = Address( sender.GetA(), sender.GetB(), sender.GetC(), sender.GetD(), packet_ServerPort );
            entry.timeoutAccumulator = 0.0f;
            ListenerEntry * existingEntry = FindEntry( entry );
            if ( existingEntry ){
                existingEntry->timeoutAccumulator = 0.0f;}
            else
                entries.push_back( entry );
        }
        std::vector<ListenerEntry>::iterator itor = entries.begin();
        
        while ( itor != entries.end() ) {
            itor->timeoutAccumulator += (float)deltaTime;
            if ( itor->timeoutAccumulator > timeout )
                itor = entries.erase( itor );
            else
                ++itor;
        }
    }
    
    ListenerEntry * Listener::FindEntry( const ListenerEntry & entry ) {
        for ( int i = 0; i < (int) entries.size(); ++i ) {
            if ( entries[i].address == entry.address && strcmp( entries[i].name, entry.name ) == 0 )
                return &entries[i];
        }
        return NULL;
    }

}
