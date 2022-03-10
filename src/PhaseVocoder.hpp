//
//  PhaseVocoder.hpp
//  sing-to-me
//
//  Created by Adam Cole on 3/10/22.
//

#ifndef PhaseVocoder_hpp
#define PhaseVocoder_hpp

#include <stdio.h>
#include "ofxMaxim.h"
#include "CircularBuffer.hpp"

class PhaseVocoder {
public:
    void setup(int windowSize, int hopSize);
    void addSample(float sample);
    float readSample();

private:
    ofxMaxiFFT fft;
    
    int windowSize;
    int hopSize;
    
    CircularBuffer inputBuffer;
    CircularBuffer outputBuffer;
    
    void processWindow();
};

#endif /* PhaseVocoder_hpp */
