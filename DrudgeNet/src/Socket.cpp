#include "Socket.h"
#include <stdio.h>
#include <cassert>
#include <algorithm>

#ifdef _WIN32
#else
#include <unistd.h>
#endif

namespace Net
{
    Socket::Socket( int options ) :
    _options(options),
    _socket(0)
    {}
    
    Socket::~Socket()
    {
        Close();
    }
    
    bool Socket::Open( unsigned short port ) {
        assert( !IsOpen() );
        
        // Create socket
        _socket = ::socket(AF_INET,
                           SOCK_DGRAM,
                           IPPROTO_UDP);
        
        if ( socket <= 0 ) {
            printf( "failed to create socket\n" );
            _socket = 0;
            return false;
        }
        
        // Bind to port
        sockaddr_in address;
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons( (unsigned short) port );
        
        if (bind(_socket, (const sockaddr*) &address, sizeof(sockaddr_in)) < 0)
        {
            printf( "failed to bind socket\n" );
            Close();
            return false;
        }
        
        // Set non-blocking IO
        if ( _options & NonBlocking )
        {
#if PLATFORM == PLATFORM_MAC || PLATFORM == PLATFORM_UNIX
            
            int nonBlocking = 1;
            if (fcntl(_socket, F_SETFL, O_NONBLOCK, nonBlocking) == -1)
            {
                printf( "failed to set non-blocking socket\n" );
                Close();
                return false;
            }
            
#elif PLATFORM == PLATFORM_WINDOWS
            
            DWORD nonBlocking = 1;
            if (ioctlsocket(socket, FIONBIO, &nonBlocking ) != 0)
            {
                printf( "failed to set non-blocking socket\n" );
                Close();
                return false;
            }
            
#endif
        }
        
        // Set broadcast socket
        if ( _options & Broadcast ) {
            int enable = 1;
            if ( setsockopt( _socket, SOL_SOCKET, SO_BROADCAST, (const char*) &enable, sizeof( enable ) ) < 0 ) {
                printf( "failed to set socket to broadcast\n" );
                Close();
                return false;
            }
        }
        
        return true;
    }
    
    void Socket::Close() {
        if ( _socket != 0 ) {
#if PLATFORM == PLATFORM_MAC || PLATFORM == PLATFORM_UNIX
            close( _socket );    // Old c++
#elif PLATFORM == PLATFORM_WINDOWS
            closesocket( _socket );
#endif
            _socket = 0;
        }
    }
    
    bool Socket::IsOpen() const {
        return _socket != 0;
    }
    
    bool Socket::Send( const Address & destination, const void * data, int size ) {
        assert( data );
        assert( size > 0 );
        
        if ( _socket == 0 )
            return false;
        
        assert( destination.GetAddress() != 0 );
        assert( destination.GetPort() != 0 );
        
        sockaddr_in address;
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = htonl( destination.GetAddress() );
        address.sin_port = htons( (unsigned short) destination.GetPort() );
        
        int sent_bytes = (int)sendto(_socket,
                                     (const char*)data,
                                     size,
                                     0,
                                     (sockaddr*)&address,
                                     sizeof(sockaddr_in));
//        printf( "socket sent:%i of %i bytes to @%d.%d.%d.%d:%i \n",
//               sent_bytes,
//               size,
//               destination.GetA(),
//               destination.GetB(),
//               destination.GetC(),
//               destination.GetD(),
//               destination.GetPort());
        return sent_bytes == size;
    }
    
    int Socket::Receive( Address & sender, void * data, int size ) {
        assert( data );
        assert( size > 0 );
        
        if ( _socket == 0 )
            return false;
        
#if PLATFORM == PLATFORM_WINDOWS
        typedef int socklen_t;
#endif
        
        sockaddr_in from;
        socklen_t fromLength = sizeof( from );
        
        int received_bytes = (int)recvfrom(_socket,
                                           (char*)data,
                                           size,
                                           0,
                                           (sockaddr*)&from,
                                           &fromLength);
        
        if ( received_bytes <= 0 )
            return 0;
        
        unsigned int address = ntohl( from.sin_addr.s_addr );
        unsigned short port = ntohs( from.sin_port );
        //printf( "socket incoming data:%i:%d", address, port );
        
        sender = Address( address, port );
//        printf( "socket read:%i of %i bytes from @%d.%d.%d.%d:%i \n",
//               received_bytes,
//               size,
//               sender.GetA(),
//               sender.GetB(),
//               sender.GetC(),
//               sender.GetD(),
//               sender.GetPort());
        return received_bytes;
    }
}

