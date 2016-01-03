#ifndef NET_STREAM_H
#define NET_STREAM_H

#include "BitPacker.h"
#include <stdio.h>
#include <cstdint>

namespace Net
{
    // stream class
    //  + unifies read and write into a serialize operation
    //  + provides attribution of stream for debugging purposes
    
    class Stream
    {
    public:
        
        enum Mode
        {
            Read,
            Write
        };
        
        Stream( Mode mode, void * buffer, int bytes, void * journal_buffer = NULL, int journal_bytes = 0 );
        
        bool SerializeBoolean( bool & value );
        
        bool SerializeByte( char & value, char min = -127, char max = +128 );
        
        bool SerializeByte( signed char & value, signed char min = -127, signed char max = +128 );
        
        bool SerializeByte( unsigned char & value, unsigned char min = 0, unsigned char max = 0xFF );
        
        bool SerializeShort( signed short & value, signed short min = -32767, signed short max = +32768 );
        
        bool SerializeShort( unsigned short & value, unsigned short min = 0, unsigned short max = 0xFFFF );
        
        bool SerializeInteger( signed int & value, signed int min = -2147483646, signed int max = +2147483647 );
        
        bool SerializeInteger( unsigned int & value, unsigned int min = 0, unsigned int max = 0xFFFFFFFF );
        
        bool SerializeFloat( float & value );
        
        bool SerializeDouble( double & value );

        bool SerializeBits( unsigned int & value, int bits );
        
        bool SerializeBits( uint64_t & value, int bits );

        bool Checkpoint();
        
        bool IsReading() const;
        
        bool IsWriting() const;
        
        int GetBitsProcessed() const;
        
        int GetBitsRemaining() const;
        
        static int BitsRequired( unsigned int minimum, unsigned int maximum );
        
        static int BitsRequired( unsigned int distinctValues );
        
        int GetDataBytes() const;
        
        int GetJournalBytes() const;
        
        void DumpJournal();
    private:
        
        BitPacker bitpacker;
        BitPacker journal;
    };
}

#endif /* NET_STREAM_H */
