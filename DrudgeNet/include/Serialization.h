#ifndef NET_SERIALIZATION_H
#define NET_SERIALIZATION_H

namespace Net
{
    namespace Serialization
    {
        inline void WriteInteger( unsigned char * data, unsigned int value ) {
            data[0] = (unsigned char) ( value >> 24 );
            data[1] = (unsigned char) ( ( value >> 16 ) & 0xFF );
            data[2] = (unsigned char) ( ( value >> 8 ) & 0xFF );
            data[3] = (unsigned char) ( value & 0xFF );
        }
        inline void ReadInteger( const unsigned char * data, unsigned int & value ) {
            value = (( (unsigned int)data[0] << 24 ) |
                     ( (unsigned int)data[1] << 16 ) |
                     ( (unsigned int)data[2] << 8 )  |
                     ( (unsigned int)data[3] ) );
        }
    }
}

#endif /* NET_SERIALIZATION_H */
