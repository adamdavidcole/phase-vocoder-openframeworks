//
//  CircularBuffer.hpp
//  sing-to-me
//
//  Created by Adam Cole on 3/10/22.
//

#ifndef CircularBuffer_hpp
#define CircularBuffer_hpp

#include <stdio.h>
#include <vector>

#define DEFAULT_CIRCULAR_BUFFER_SIZE 100

class CircularBuffer {
public:
    CircularBuffer();
    CircularBuffer(int size);
    
    int size();
    void setSize(int size);
    
    void push(float val);
    void add(float val);
    float read();
    float readAndClear();
    
    int getWriteCount();
    
    void shiftWritePoint(int shift);
    void shiftReadPoint(int shift);
    void setReadPointRelativeToWritePoint(int relativeShift);
    
    std::vector<float> getWindow(int prevSamples);
    
//private:
    std::vector<float> buffer;
    unsigned int writePoint;
    unsigned int readPoint;
    unsigned int writeCount;
    
    void _addToBuffer(float val, bool shouldAdd);
    float _read(bool shouldClearAfterRead);
};

#endif /* CircularBuffer_hpp */
