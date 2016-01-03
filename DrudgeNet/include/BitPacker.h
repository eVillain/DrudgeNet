#ifndef NET_BITPACKER_H
#define NET_BITPACKER_H

#include <cstdint>

namespace Net
{
    // bitpacker class
    //  + read and write non-8 multiples of bits efficiently
    
    class BitPacker
    {
    public:
        
        enum Mode
        {
            Read,
            Write
        };
        
        BitPacker( Mode mode, void * buffer, int bytes );
        
        void WriteBits( unsigned int value, int bits = 32 );
        
        void ReadBits( unsigned int & value, int bits = 32 );
        
        void WriteBits( uint64_t value, int bits = 64 );
        
        void ReadBits( uint64_t & value, int bits = 64 );
        
        void * GetData();
        
        int GetBits() const;
        
        int GetBytes() const;
        
        int BitsRemaining() const;
        
        Mode GetMode() const;
        
        bool IsValid() const;
    private:
        
        int bit_index;
        unsigned char * ptr;
        unsigned char * buffer;
        int bytes;
        Mode mode;
    };
}

#endif /* NET_BITPACKER_H */
