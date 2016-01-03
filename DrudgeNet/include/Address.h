#ifndef NET_ADDRESS_H
#define NET_ADDRESS_H

namespace Net
{
    class Address
    {
    public:
        
        Address() :
        _address(0),
        _port(0)
        {}
        
        Address(unsigned char a,
                unsigned char b,
                unsigned char c,
                unsigned char d,
                unsigned short port) :
        _address( ( a << 24 ) | ( b << 16 ) | ( c << 8 ) | d ),
        _port(port)
        {}
        
        Address( unsigned int address, unsigned short port )
        {
            _address = address;
            _port = port;
        }
        
        unsigned int GetAddress() const
        {
            return _address;
        }
        
        unsigned char GetA() const
        {
            return ( unsigned char ) ( _address >> 24 );
        }
        
        unsigned char GetB() const
        {
            return ( unsigned char ) ( _address >> 16 );
        }
        
        unsigned char GetC() const
        {
            return ( unsigned char ) ( _address >> 8 );
        }
        
        unsigned char GetD() const
        {
            return ( unsigned char ) ( _address );
        }
        
        unsigned short GetPort() const
        {
            return _port;
        }
        
        bool operator == ( const Address & other ) const
        {
            return _address == other._address && _port == other._port;
        }
        
        bool operator != ( const Address & other ) const
        {
            return ! ( *this == other );
        }
        
        // note: required so we can use an Address as a key in std::map
        bool operator < ( const Address & other ) const
        {
            if ( _address < other._address ) {
                return true;
            }
            if ( _address > other._address ) {
                return false;
            } else {
                return (_port < other._port);
            }
        }
    private:
        
        unsigned int _address;
        unsigned short _port;
    };
}

#endif
