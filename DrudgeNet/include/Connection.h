#ifndef NET_CONNECTION_H
#define NET_CONNECTION_H

#include "Socket.h"

namespace Net
{
    class Connection
    {
    public:
        
        enum Mode
        {
            None,
            Client,
            Server
        };
        
        Connection( unsigned int protocolId, float timeout );
        
        virtual ~Connection();
        
        bool Start( int port );
        
        void Stop();
        
        bool IsRunning() const { return running; }
        
        void Listen();
        
        void Connect( const Address & address );
        
        bool IsConnecting() const { return state == Connecting; }
        
        bool ConnectFailed() const { return state == ConnectFail; }
        
        bool IsConnected() const { return state == Connected; }
        
        bool IsListening() const { return state == Listening; }
        
        Mode GetMode() const { return mode; }
        
        virtual void Update( float deltaTime );
        
        virtual bool SendPacket( const unsigned char data[], int size );
        
        virtual int ReceivePacket( unsigned char data[], int size );
        
        int GetHeaderSize() const { return 4; }
        
    protected:
        
        virtual void OnStart()		{}
        virtual void OnStop()		{}
        virtual void OnConnect()    {}
        virtual void OnDisconnect() {}
        
    private:
        
        void ClearData();
        
        enum State
        {
            Disconnected,
            Listening,
            Connecting,
            ConnectFail,
            Connected
        };
        
        unsigned int protocolId;
        float timeout;
        
        bool running;
        Mode mode;
        State state;
        Socket socket;
        float timeoutAccumulator;
        Address address;
    };
}


#endif /* Connection_hpp */
