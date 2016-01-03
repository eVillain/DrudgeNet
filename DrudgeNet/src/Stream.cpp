#include "Stream.h"
#include <cassert>

namespace Net
{    
    Stream::Stream( Mode mode, void * buffer, int bytes, void * journal_buffer, int journal_bytes )
    : bitpacker( mode == Write ? BitPacker::Write : BitPacker::Read, buffer, bytes ),
    journal( mode == Write ? BitPacker::Write : BitPacker::Read, journal_buffer, journal_bytes )
    {
    }
    
    bool Stream::SerializeBoolean( bool & value )
    {
        unsigned int tmp = (unsigned int) value;
        bool result = SerializeBits( tmp, 1 );
        value = (bool) tmp;
        return result;
    }
    
    bool Stream::SerializeByte( char & value, char min, char max )
    {
        // wtf: why do I have to do this!?
        unsigned int tmp = (unsigned int) ( value + 127 );
        bool result = SerializeInteger( tmp, (unsigned int ) ( min + 127 ), ( max + 127 ) );
        value = ( (char) tmp ) - 127;
        return result;
    }
    
    bool Stream::SerializeByte( signed char & value, signed char min, signed char max )
    {
        unsigned int tmp = (unsigned int) ( value + 127 );
        bool result = SerializeInteger( tmp, (unsigned int ) ( min + 127 ), ( max + 127 ) );
        value = ( (signed char) tmp ) - 127;
        return result;
    }
    
    bool Stream::SerializeByte( unsigned char & value, unsigned char min, unsigned char max )
    {
        unsigned int tmp = (unsigned int) value;
        bool result = SerializeInteger( tmp, min, max );
        value = (unsigned char) tmp;
        return result;
    }
    
    bool Stream::SerializeShort( signed short & value, signed short min, signed short max )
    {
        unsigned int tmp = (unsigned int) ( value + 32767 );
        bool result = SerializeInteger( tmp, (unsigned int ) ( min + 32767 ), ( max + 32767 ) );
        value = ( (signed short) tmp ) - 32767;
        return result;
    }
    
    bool Stream::SerializeShort( unsigned short & value, unsigned short min, unsigned short max )
    {
        unsigned int tmp = (unsigned int) value;
        bool result = SerializeInteger( tmp, min, max );
        value = (unsigned short) tmp;
        return result;
    }
    
    bool Stream::SerializeInteger( signed int & value, signed int min, signed int max )
    {
        unsigned int tmp = (unsigned int) ( value + 2147483646 );
        bool result = SerializeInteger( tmp, (unsigned int ) ( min + 2147483646 ), ( max + 2147483646 ) );
        value = ( (signed int) tmp ) - 2147483646;
        return result;
    }
    
    bool Stream::SerializeInteger( unsigned int & value, unsigned int min, unsigned int max )
    {
        assert( min < max );
        if ( IsWriting() )
        {
            assert( value >= min );
            assert( value <= max );
        }
        const int bits_required = BitsRequired( min, max );
        unsigned int bits = value - min;
        bool result = SerializeBits( bits, bits_required );
        if ( IsReading() )
        {
            value = bits + min;
            assert( value >= min );
            assert( value <= max );
        }
        return result;
    }
    
    bool Stream::SerializeFloat( float & value )
    {
        union FloatInt
        {
            unsigned int i;
            float f;
        };
        if ( IsReading() )
        {
            FloatInt floatInt;
            if ( !SerializeBits( floatInt.i, 32 ) )
                return false;
            value = floatInt.f;
            return true;
        }
        else
        {
            FloatInt floatInt;
            floatInt.f = value;
            return SerializeBits( floatInt.i, 32 );
        }
    }
    
    bool Stream::SerializeDouble( double & value )
    {
        union DoubleInt64
        {
            uint64_t i;
            double d;
        };
        if ( IsReading() )
        {
            DoubleInt64 doubleInt;
            if ( !SerializeBits( doubleInt.i, 64 ) )
                return false;
            value = doubleInt.d;
            return true;
        }
        else
        {
            DoubleInt64 doubleInt;
            doubleInt.d = value;
            return SerializeBits( doubleInt.i, 64 );
        }
    }
    
    bool Stream::SerializeBits( unsigned int & value, int bits )
    {
        assert( bits >= 1 );
        assert( bits <= 32 );
        if ( bitpacker.BitsRemaining() < bits )
            return false;
        if ( journal.IsValid() )
        {
            unsigned int token = 2 + bits;		// note: 0 = end, 1 = checkpoint, [2,34] = n - 2 bits written
            if ( IsWriting() )
            {
                journal.WriteBits( token, 6 );
            }
            else
            {
                journal.ReadBits( token, 6 );
                int bits_written = token - 2;
                if ( bits != bits_written )
                {
                    printf( "desync read/write: attempting to read %d bits when %d bits were written\n", bits, bits_written );
                    return false;
                }
            }
        }
        if ( IsReading() )
            bitpacker.ReadBits( value, bits );
        else
            bitpacker.WriteBits( value, bits );
        return true;
    }
    
    bool Stream::SerializeBits( uint64_t & value, int bits )
    {
        assert( bits >= 1 );
        assert( bits <= 64 );
        if ( bitpacker.BitsRemaining() < bits )
            return false;
        if ( journal.IsValid() )
        {
            unsigned int token = 2 + bits;		// note: 0 = end, 1 = checkpoint, [2,66] = n - 2 bits written
            if ( IsWriting() )
            {
                journal.WriteBits( token, 6 );
            }
            else
            {
                journal.ReadBits( token, 6 );
                int bits_written = token - 2;
                if ( bits != bits_written )
                {
                    printf( "desync read/write: attempting to read %d bits when %d bits were written\n", bits, bits_written );
                    return false;
                }
            }
        }
        if ( IsReading() )
            bitpacker.ReadBits( value, bits );
        else
            bitpacker.WriteBits( value, bits );
        return true;
    }
    
    bool Stream::Checkpoint()
    {
        if ( journal.IsValid() )
        {
            unsigned int token = 1;		// note: 0 = end, 1 = checkpoint, [2,34] = n - 2 bits written
            if ( IsWriting() )
            {
                journal.WriteBits( token, 6 );
            }
            else
            {
                journal.ReadBits( token, 6 );
                if ( token != 1 )
                {
                    printf( "desync read/write: checkpoint not present in journal\n" );
                    return false;
                }
            }
        }
        unsigned int magic = 0x12345678;
        unsigned int value = magic;
        if ( bitpacker.BitsRemaining() < 32 )
        {
            printf( "not enough bits remaining for checkpoint\n" );
            return false;
        }
        if ( IsWriting() )
            bitpacker.WriteBits( value, 32 );
        else
            bitpacker.ReadBits( value, 32 );
        if ( value != magic )
        {
            printf( "checkpoint failed!\n" );
            return false;
        }
        return true;
    }
    
    bool Stream::IsReading() const
    {
        return bitpacker.GetMode() == BitPacker::Read;
    }
    
    bool Stream::IsWriting() const
    {
        return bitpacker.GetMode() == BitPacker::Write;
    }
    
    int Stream::GetBitsProcessed() const
    {
        return bitpacker.GetBits();
    }
    
    int Stream::GetBitsRemaining() const
    {
        return bitpacker.BitsRemaining();
    }
    
    int Stream::BitsRequired( unsigned int minimum, unsigned int maximum )
    {
        assert( maximum > minimum );
        assert( maximum >= 1 );
        if ( maximum - minimum >= 0x7FFFFFF )
            return 32;
        return BitsRequired( maximum - minimum + 1 );
    }
    
    int Stream::BitsRequired( unsigned int distinctValues )
    {
        assert( distinctValues > 1 );
        unsigned int maximumValue = distinctValues - 1;
        for ( int index = 0; index < 32; ++index )
        {
            if ( ( maximumValue & ~1 ) == 0 )
                return index + 1;
            maximumValue >>= 1;
        }
        return 32;
    }
    
    int Stream::GetDataBytes() const
    {
        return bitpacker.GetBytes();
    }
    
    int Stream::GetJournalBytes() const
    {
        return journal.GetBytes();
    }
    
    void Stream::DumpJournal()
    {
        if ( journal.IsValid() )
        {
            printf( "-----------------------------\n" );
            printf( "dump journal:\n" );
            BitPacker reader( BitPacker::Read, journal.GetData(), journal.GetBytes() );
            while ( reader.BitsRemaining() > 6 )
            {
                unsigned int token = 0;
                reader.ReadBits( token, 6 );
                if ( token == 0 )
                    break;
                if ( token == 1 )
                    printf( " (checkpoint)\n" );
                else
                    printf( " + %d bits\n", token - 2 );
            }
            printf( "-----------------------------\n" );
        }
        else
            printf( "no journal exists!\n" );
    }
}