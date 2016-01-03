#include "Beacon.h"
#include "Socket.h"
#include "Serialization.h"
#include <string.h>
#include <stdio.h>
#include <assert.h>

namespace Net
{
    Beacon::Beacon(const char name[],
                   unsigned int protocol,
                   unsigned int listenPort,
                   unsigned int hostPort)
    : socket( Socket::Broadcast | Socket::NonBlocking ) {
#ifdef _WIN32
        strncpy_s( this->name, name, 64 );
#else
        strncpy(this->name, name, 64);
#endif
        this->name[64] = '\0';
        this->protocolID = protocol;
        this->listenerPort = listenPort;
        this->serverPort = hostPort;
        running = false;
    }
    
    Beacon::~Beacon() {
        if ( running )
            Stop();
    }
    
    bool Beacon::Start( int port ) {
        assert( !running );
        printf( "Beacon: start on port %d\n", port );
        if ( !socket.Open( port ) )
            return false;
        running = true;
        return true;
    }
    
    void Beacon::Stop() {
        assert( running );
        printf( "Beacon: stop\n" );
        socket.Close();
        running = false;
    }
    
    void Beacon::Update( double delta ) {
        assert( running );
        timeAccumulator += delta;
        if ( timeAccumulator >= 1.0f ) {
            // Broadcast beacon advertising server
            unsigned char *packet;
            packet = new unsigned char[12+1+64];
            Serialization::WriteInteger( packet, 0 );
            Serialization::WriteInteger( packet + 4, protocolID );
            Serialization::WriteInteger( packet + 8, serverPort );
            packet[12] = (unsigned char) strlen( name );
            assert( packet[12] < 63);
            memcpy( packet + 13, name, strlen( name ) );
//            printf( "Beacon: broadcasting...\n" );
            if ( !socket.Send( Address(255,255,255,255,listenerPort), packet, 12 + 1 + packet[12] ) ) {
                printf( "Beacon: failed to send broadcast packet\n" );
            }
            delete [] packet;
            timeAccumulator -= 1.0f;
        }
    }
}

