//
//  PhaseVocoder.hpp
//  sing-to-me
//
//  Created by Adam Cole on 3/10/22.
//

#ifndef PhaseVocoder_hpp
#define PhaseVocoder_hpp

#include <stdio.h>
#include "ofMain.h"
#include "ofxMaxim.h"
#include "CircularBuffer.hpp"

class PhaseVocoder;

class WindowProcessor : public ofThread {
public:
    void setup(PhaseVocoder* _phaseVocoder);
    void threadedFunction();
    
    PhaseVocoder* phaseVocoder;
};


class PhaseVocoder {
public:
    ~PhaseVocoder();
    void setup(int fftSize, int windowSize, int hopSize);
    void addSample(float sample);
    float readSample();
    void processWindow();

    ofxMaxiFFT fft;

private:
    
    int fftSize;
    int windowSize;
    int hopSize;
    
    CircularBuffer* inputBuffer;
    CircularBuffer* outputBuffer;
    
    vector<float> nextWindowToProcess;
    
    WindowProcessor windowProcessor;
    
    
};

#endif /* PhaseVocoder_hpp */
