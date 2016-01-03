#ifndef NET_SOCKET_H
#define NET_SOCKET_H

#include "SocketPlatform.h"
#include "Address.h"

namespace Net
{
    class Socket
    {
    public:
        
        enum SocketOptions {
            NonBlocking = 1,
            Broadcast = 2
        };
        
        Socket( const int options = NonBlocking );
        
        ~Socket();
        
        bool Open( unsigned short port );
        
        void Close();
        
        bool IsOpen() const;
        
        bool Send( const Address & destination, const void * data, int size );
        
        int Receive( Address & sender, void * data, int size );
        
    private:
        int _socket;
        int _options;
    };
}

inline bool InitializeSockets() {
#if PLATFORM == PLATFORM_WINDOWS
    WSADATA WsaData;
    
    int err_code = WSAStartup( MAKEWORD(2,2), &WsaData ) != NO_ERROR;
    
    // I get error from outputdebug string using sysinternal debugview tool.
    switch(err_code)
    {
        case WSASYSNOTREADY:
            OutputDebugStringW(L"The underlying network subsystem is not ready for network communication.");
            break;
        case WSAVERNOTSUPPORTED:
            OutputDebugStringW(L"The version of Windows Sockets support requested is not provided by this particular Windows Sockets implementation.");
            break;
        case WSAEINPROGRESS:
            OutputDebugStringW(L"A blocking Windows Sockets 1.1 operation is in progress");
            break;
        case WSAEPROCLIM:
            OutputDebugStringW(L"A limit on the number of tasks supported by the Windows Sockets implementation has been reached.");
            break;
        case WSAEFAULT:
            OutputDebugStringW(L"The lpWSAData parameter is not a valid pointer.");
            break;
        default:
            break;
    }
    return err_code == 0;
#else
    return true;
#endif
}

inline void ShutdownSockets() {
#if PLATFORM == PLATFORM_WINDOWS
    WSACleanup();
#endif
}

#endif
