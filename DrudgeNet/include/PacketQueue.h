#ifndef NET_PACKET_QUEUE_H
#define NET_PACKET_QUEUE_H

#include <list>
#include <assert.h>

namespace Net
{
    
    struct PacketData
    {
        unsigned int sequence;			// packet sequence number
        float time;					    // time offset since packet was sent or received (depending on context)
        int size;						// packet size in bytes
    };
    
    inline bool IsSequenceMoreRecent( unsigned int s1, unsigned int s2, unsigned int max_sequence )
    {
        return (( s1 > s2 ) && ( s1 - s2 <= max_sequence/2 )) || ((( s2 > s1 ) && ( s2 - s1 > max_sequence/2 )));
    }

    // packet queue to store information about sent and received packets sorted in sequence order
    //  + we define ordering using the "IsSequenceMoreRecent" function,
    // this works provided there is a large gap when sequence wrap occurs
    
    class PacketQueue : public std::list<PacketData>
    {
    public:
        
        bool Exists( unsigned int sequence ) {
            for ( iterator itor = begin(); itor != end(); ++itor )
                if ( itor->sequence == sequence )
                    return true;
            return false;
        }
        
        void InsertSorted( const PacketData & p, unsigned int max_sequence ) {
            if ( empty() ) {
                push_back( p );
            }
            else {
                if ( !IsSequenceMoreRecent( p.sequence, front().sequence, max_sequence ) ) {
                    push_front( p );
                }
                else if ( IsSequenceMoreRecent( p.sequence, back().sequence, max_sequence ) ) {
                    push_back( p );
                }
                else {
                    for ( PacketQueue::iterator itor = begin(); itor != end(); itor++ ) {
                        assert( itor->sequence != p.sequence );
                        if ( IsSequenceMoreRecent( itor->sequence, p.sequence, max_sequence ) ) {
                            insert( itor, p );
                            break;
                        }
                    }
                }
            }
        }
        
        void VerifySorted( unsigned int max_sequence ) {
            PacketQueue::iterator prev = end();
            for ( PacketQueue::iterator itor = begin(); itor != end(); itor++ ) {
                assert( itor->sequence <= max_sequence );
                if ( prev != end() ) {
                    assert( IsSequenceMoreRecent( itor->sequence, prev->sequence, max_sequence ) );
                    prev = itor;
                }
            }
        }
    };
}

#endif
