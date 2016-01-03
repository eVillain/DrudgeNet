#include "Transport.h"
#include "TransportLAN.h"
#include <assert.h>

namespace Net
{
    static TransportType transportType = Transport_None;
    static int transportCount = 0;
    
    bool Transport::Initialize( TransportType type )
    {
        assert( type != Transport_None );
        bool result = false;
        switch ( type )
        {
            case Transport_LAN: result = TransportLAN::Initialize(); break;
            default: break;
        }
        transportType = type;
        return result;
    }
    
    void Transport::Shutdown()
    {
        switch ( transportType )
        {
            case Transport_LAN: TransportLAN::Shutdown();
            default: break;
        }
    }
    
    Transport * Transport::Create()
    {
        Transport * transport = nullptr;
        
        assert( transportCount >= 0 );
        
        switch ( transportType )
        {
            case Transport_LAN: 	transport = new TransportLAN(); 		break;
                //		case Transport_RakNet:	transport = new TransportRakNet(); 		break;
                //		case Transport_OpenTNL:	transport = new TransportOpenTNL();		break;
                //		case Transport_eNet:	transport = new TransportENet();		break;
            default: break;
        }
        
        assert( transport );
        assert( transport->GetType() == transportType );
        
        transportCount++;
        
        return transport;
    }
    
    void Transport::Destroy( Transport * transport )
    {
        assert( transport );
        assert( transport->GetType() == transportType );
        assert( transportCount > 0 );
        delete transport;
        transportCount--;
    }
}
