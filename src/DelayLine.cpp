//
//  DelayLine.cpp
//  sing-to-me
//
//  Created by Adam Cole on 3/23/22.
//

#include "DelayLine.hpp"

void DelayLine::setup(float _delayTime, float sampleRate) {
    buffer.setSize(sampleRate * 10);
    
    float delayTime = _delayTime;
    if (delayTime > 10) {
        delayTime = 10;
    }
    
    buffer.shiftWritePoint(sampleRate * delayTime);
}

void DelayLine::addSample(float sample) {
    buffer.push(sample);
}

float DelayLine::getSample() {
    return buffer.readAndClear();
}
