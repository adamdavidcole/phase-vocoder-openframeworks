//
//  CircularBuffer.cpp
//  sing-to-me
//
//  Created by Adam Cole on 3/10/22.
//
#include <iostream>
#include "CircularBuffer.hpp"

CircularBuffer::CircularBuffer() : CircularBuffer(DEFAULT_CIRCULAR_BUFFER_SIZE) {}

CircularBuffer::CircularBuffer(int size) {
    buffer.resize(size);
    writePoint = 0;
    readPoint = 0;
    writeCount = 0;
}

int CircularBuffer::size() {
    return buffer.size();
}

void CircularBuffer::setSize(int size) {
    buffer.resize(size);
}

void CircularBuffer::push(float val) {
    _addToBuffer(val, false);
}

void CircularBuffer::add(float val) {
    _addToBuffer(val, true);
}

void CircularBuffer::_addToBuffer(float val, bool shouldAdd) {
    if (buffer.size() == 0) return;
    
    if (shouldAdd) {
        float currVal = buffer[writePoint];
        buffer[writePoint] = (val + currVal) / 2.0;
    } else {
        buffer[writePoint] = val;
    }
    
    writePoint += 1;
    writeCount += 1;
    
    if (writePoint >= buffer.size()) {
        writePoint = 0;
    }
}

float CircularBuffer::read() {
    return _read(false);
}

float CircularBuffer::readAndClear() {
    return _read(true);
}

float CircularBuffer::_read(bool shouldClearAfterRead) {
    if (buffer.size() == 0) return 0.0;
    
    float val = buffer[readPoint];
    if (shouldClearAfterRead) {
        buffer[readPoint] = 0;
    }
    
    readPoint += 1;
    
    if (readPoint >= buffer.size()) {
        readPoint = 0;
    }
    
    return val;
}

int CircularBuffer::getWriteCount() {
    return writeCount;
}


std::vector<float> CircularBuffer::getWindow(int prevSamplesCount) {
    std::vector<float> window(prevSamplesCount);
    int lastWritePoint = writePoint;
    
    for (int i = 0; i < prevSamplesCount; i++) {
        int bufferIndex = (lastWritePoint - prevSamplesCount + i) % buffer.size();
        bufferIndex = abs(bufferIndex);
        float bufferVal = buffer[bufferIndex];
        window[i] = bufferVal;
    }
    
    return window;
}

void CircularBuffer::shiftWritePoint(int shift) {
    int nextWritePoint = (writePoint + shift) % buffer.size();
    writePoint = abs(nextWritePoint);
}
void CircularBuffer::shiftReadPoint(int shift) {
    int nextReadPoint = (readPoint + shift) % buffer.size();
    readPoint = abs(nextReadPoint);
}
void CircularBuffer::setReadPointRelativeToWritePoint(int relativeShift) {
    if (relativeShift < 0) return;
    
    int nextReadPoint = (writePoint - relativeShift) % buffer.size();
    readPoint = abs(nextReadPoint);
}
