#include "Connection.h"
#include <cassert>
#include <stdio.h>
#include <algorithm>

namespace Net
{
    Connection::Connection( unsigned int protocolId, float timeout )
    {
        this->protocolId = protocolId;
        this->timeout = timeout;
        mode = None;
        running = false;
        ClearData();
    }
    
    Connection::~Connection()
    {
        if ( IsRunning() )
            Stop();
    }
    
    bool Connection::Start( int port )
    {
        assert( !running );
        printf( "Connection: start connection on port %d\n", port );
        if ( !socket.Open( port ) )
            return false;
        running = true;
        OnStart();
        return true;
    }
    
    void Connection::Stop()
    {
        assert( running );
        printf( "Connection: stop connection\n" );
        bool connected = IsConnected();
        ClearData();
        socket.Close();
        running = false;
        if ( connected )
            OnDisconnect();
        OnStop();
    }
    
    void Connection::Listen()
    {
        printf( "Connection: server listening for connection\n" );
        bool connected = IsConnected();
        ClearData();
        if ( connected )
            OnDisconnect();
        mode = Server;
        state = Listening;
    }
    
    void Connection::Connect( const Address & address )
    {
        printf( "Connection: client connecting to %d.%d.%d.%d:%d\n",
               address.GetA(), address.GetB(), address.GetC(), address.GetD(), address.GetPort() );
        bool connected = IsConnected();
        ClearData();
        if ( connected )
            OnDisconnect();
        mode = Client;
        state = Connecting;
        this->address = address;
    }
    
    
    void Connection::Update( float deltaTime )
    {
        assert( running );
        timeoutAccumulator += deltaTime;
        if ( timeoutAccumulator > timeout )
        {
            if ( state == Connecting )
            {
                printf( "Connection: connect timed out\n" );
                ClearData();
                state = ConnectFail;
                OnDisconnect();
            }
            else if ( state == Connected )
            {
                printf( "Connection: connection timed out\n" );
                ClearData();
                if ( state == Connecting )
                    state = ConnectFail;
                OnDisconnect();
            }
        }
    }
    
    bool Connection::SendPacket( const unsigned char data[], int size )
    {
        assert( running );
        if ( address.GetAddress() == 0 )
            return false;
        unsigned char* packet = new unsigned char[size+4];
        packet[0] = (unsigned char) ( protocolId >> 24 );
        packet[1] = (unsigned char) ( ( protocolId >> 16 ) & 0xFF );
        packet[2] = (unsigned char) ( ( protocolId >> 8 ) & 0xFF );
        packet[3] = (unsigned char) ( ( protocolId ) & 0xFF );
        memcpy( &packet[4], data, size );
        bool res = socket.Send( address, packet, size + 4 );
        delete [] packet;
        return res;
    }
    
    int Connection::ReceivePacket( unsigned char data[], int size )
    {
        assert( running );
        unsigned char* packet = new unsigned char[size+4];
        Address sender;
        int bytes_read = socket.Receive( sender, packet, size + 4 );
        if ( bytes_read == 0 )
        {
            delete [] packet;
            return 0;
        }
        if ( bytes_read <= 4 )
        {
            delete [] packet;
            return 0;
        }
        if ( packet[0] != (unsigned char) ( protocolId >> 24 ) ||
            packet[1] != (unsigned char) ( ( protocolId >> 16 ) & 0xFF ) ||
            packet[2] != (unsigned char) ( ( protocolId >> 8 ) & 0xFF ) ||
            packet[3] != (unsigned char) ( protocolId & 0xFF ) )
        {
            delete [] packet;
            return 0;
        }
        if ( mode == Server && !IsConnected() )
        {
            printf( "Connection: server accepts connection from client %d.%d.%d.%d:%d\n",
                   sender.GetA(), sender.GetB(), sender.GetC(), sender.GetD(), sender.GetPort() );
            state = Connected;
            address = sender;
            OnConnect();
        }
        if ( sender == address )
        {
            if ( mode == Client && state == Connecting )
            {
                printf( "Connection: client completes connection with server\n" );
                state = Connected;
                OnConnect();
            }
            timeoutAccumulator = 0.0f;
            memcpy( data, &packet[4], bytes_read - 4 );
            delete [] packet;
            return bytes_read - 4;
        }
        delete [] packet;
        return 0;
    }
    
    void Connection::ClearData()
    {
        state = Disconnected;
        timeoutAccumulator = 0.0f;
        address = Address();
    }
}
