#ifndef NET_BEACON_H
#define NET_BEACON_H

#include "Socket.h"

namespace Net
{
    // Beacon
    //  + sends broadcast UDP packets to the LAN
    //  + use a beacon to advertise the existence of a Server

    class Beacon
    {
    public:
        Beacon(const char hostName[],
               unsigned int protocol,
               unsigned int listenPort,
               unsigned int hostPort);
        
        ~Beacon();
        
        bool Start( int port );
        
        void Stop();
        
        void Update( double delta );
        
    private:
        
        char name[64+1];
        unsigned int protocolID;
        unsigned int listenerPort;
        unsigned int serverPort;
        bool running;
        Socket socket;
        double timeAccumulator;
    };
}

#endif
