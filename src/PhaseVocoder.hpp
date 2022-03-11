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
    vector<float> calculationsForGui;

private:
    
    int fftSize;
    int windowSize;
    int hopSize;
    
    CircularBuffer* inputBuffer;
    CircularBuffer* outputBuffer;
    
    // hold the phases from the previous hop of input samples
    vector<float> lastInputPhases;
    // These containers hold the converted representations from magnitude-phase
    // to magnitude-frequency, used for pitch shifting
    vector<float> analysisMagnitudes;
    vector<float> analysisFrequencies;
    // Hann window values
    vector<float> analysisWindowBuffer;
    
    vector<float> nextWindowToProcess;
    
    WindowProcessor windowProcessor;
    
    
};

#endif /* PhaseVocoder_hpp */
