#include "BitPacker.h"
#include <cassert>
#include <algorithm>

namespace Net
{
    BitPacker::BitPacker( Mode mode, void * buffer, int bytes )
    {
        assert( bytes >= 0 );
        this->mode = mode;
        this->buffer = (unsigned char*) buffer;
        this->ptr = (unsigned char*) buffer;
        this->bytes = bytes;
        bit_index = 0;
        if ( mode == Write )
            memset( buffer, 0, bytes );
    }
    
    void BitPacker::WriteBits( unsigned int value, int bits )
    {
        assert( ptr );
        assert( buffer );
        assert( bits > 0 );
        assert( bits <= 32 );
        assert( mode == Write );
        if ( bits < 32 )
        {
            const unsigned int mask = ( 1 << bits ) - 1;
            value &= mask;
        }
        do
        {
            assert( ptr - buffer < bytes );
            *ptr |= (unsigned char) ( value << bit_index );
            assert( bit_index < 8 );
            const int bits_written = std::min( bits, 8 - bit_index );
            assert( bits_written > 0 );
            assert( bits_written <= 8 );
            bit_index += bits_written;
            if ( bit_index >= 8 )
            {
                ptr++;
                bit_index = 0;
                value >>= bits_written;
            }
            bits -= bits_written;
            assert( bits >= 0 );
            assert( bits <= 32 );
        }
        while ( bits > 0 );
    }
    
    void BitPacker::WriteBits( uint64_t value, int bits )
    {
        uint32_t x = (uint32_t)(value>>32);
        uint32_t y = (uint32_t)value;
        WriteBits(x);
        WriteBits(y);
    }
    
    void BitPacker::ReadBits( uint64_t & value, int bits )
    {
        uint32_t x, y;
        ReadBits(x, 32);
        ReadBits(y, 32);
        value = ((uint64_t)x) << 32 | y;
    }
    
    void BitPacker::ReadBits( unsigned int & value, int bits )
    {
        assert( ptr );
        assert( buffer );
        assert( bits > 0 );
        assert( bits <= 32 );
        assert( mode == Read );
        int original_bits = bits;
        int value_index = 0;
        value = 0;
        do
        {
            assert( ptr - buffer < bytes );
            assert( bits >= 0 );
            assert( bits <= 32 );
            int bits_to_read = std::min( 8 - bit_index, bits );
            assert( bits_to_read > 0 );
            assert( bits_to_read <= 8 );
            value |= ( *ptr >> bit_index ) << value_index;
            bits -= bits_to_read;
            bit_index += bits_to_read;
            value_index += bits_to_read;
            assert( value_index >= 0 );
            assert( value_index <= 32 );
            if ( bit_index >= 8 )
            {
                ptr++;
                bit_index = 0;
            }
        }
        while ( bits > 0 );
        if ( original_bits < 32 )
        {
            const unsigned int mask = ( 1 << original_bits ) - 1;
            value &= mask;
        }
    }
    
    void * BitPacker::GetData()
    {
        return buffer;
    }
    
    int BitPacker::GetBits() const
    {
        return (int)( ptr - buffer ) * 8 + bit_index;
    }
    
    int BitPacker::GetBytes() const
    {
        return (int)( ptr - buffer ) + ( bit_index > 0 ? 1 : 0 );
    }
    
    int BitPacker::BitsRemaining() const
    {
        return (int)(bytes * 8 - ( ( ptr - buffer ) * 8 + bit_index ));
    }
    
    BitPacker::Mode BitPacker::GetMode() const
    {
        return mode;
    }
    
    bool BitPacker::IsValid() const
    {
        return buffer != NULL;
    }
}