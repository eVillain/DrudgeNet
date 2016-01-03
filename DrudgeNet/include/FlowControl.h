#ifndef NET_FLOWCONTROL_H
#define NET_FLOWCONTROL_H

namespace Net
{
    // simple binary flow control
    //  + two modes of operation: good and bad mode
    //  + if RTT exceeds 250ms drop to bad mode immediately
    //  + if RTT is under 250ms for a period of time, return to good mode

    class FlowControl
    {
    public:
        
        FlowControl();
        
        void Reset();
        
        void Update( float deltaTime, float rtt );
        
        inline float GetSendRate() { return mode == Good ? 30.0f : 10.0f; };
        
    private:
        
        enum FlowMode {
            Good,
            Bad
        };
        
        FlowMode mode;
        float penalty_time;
        float good_conditions_time;
        float penalty_reduction_accumulator;
    };
}

#endif
