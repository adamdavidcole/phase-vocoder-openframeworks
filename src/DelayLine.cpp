//
//  DelayLine.cpp
//  sing-to-me
//
//  Created by Adam Cole on 3/23/22.
//
#include <iostream>
#include <math.h>
#include "DelayLine.hpp"

void DelayLine::setup(float _delayTime, float sampleRate) {
    int MAX_DELAY = 3 * sampleRate;

    buffer = new CircularBuffer(MAX_DELAY);
//    buffer.setSize(sampleRate * MAX_DELAY);
    
    int delayTime = floor(_delayTime);
    if (delayTime > MAX_DELAY) {
        delayTime = MAX_DELAY;
    }
    
//    buffer->shiftWritePoint(sampleRate * delayTime);
    buffer->shiftWritePoint(delayTime);
}

float DelayLine::delay(float sample) {
    buffer->push(sample);
    return buffer->readAndClear();
}

float DelayLine::updateDelayTime(float delayTime) {
    buffer->setReadPointRelativeToWritePoint(delayTime);
}

void DelayLine::addSample(float sample) {
    buffer->push(sample);
}

float DelayLine::getSample() {
    return buffer->read();
}
