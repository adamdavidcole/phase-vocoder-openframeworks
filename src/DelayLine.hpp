//
//  DelayLine.hpp
//  sing-to-me
//
//  Created by Adam Cole on 3/23/22.
//

#ifndef DelayLine_hpp
#define DelayLine_hpp

#include <stdio.h>
#include "CircularBuffer.hpp"


class DelayLine {
public:
    void setup(float delayTime, float sampleRate);
    void addSample(float sample);
    float getSample();
    
private:
    CircularBuffer buffer;
};

#endif /* DelayLine_hpp */
