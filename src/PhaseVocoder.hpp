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
#include "ofxFft.h"
#include "CircularBuffer.hpp"
#include "DelayLine.hpp"

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
    void processBlock(float* amplitudes, float* phases);
    
    void processBlockWithPitchShift(float* amplitudes, float* phases);
    void processBlockWithDelay(float* amplitudes, float* phases);
    void processBlockWithCrossover(float* amplitudes, float* phases, float* crossOverSampleAmplitudes, float* crossOverSamplePhases);
    void setPitchShift(float pitchShift);

    float pitchShift;
    
    ofxFft* fft;
    ofxFft* crossOverSampleFft;
    float* signalFftAmplitudes;
    float* processedFftAmplitudes;
    
    int pitchCount;
    
    void setCrossOverSample(maxiSample sample);

private:
    
    int fftSize;
    int windowSize;
    int hopSize;
    
    CircularBuffer* inputBuffer;
    CircularBuffer* outputBuffer;
    
    CircularBuffer* crossOverSampleInputBuffer;
    
    // hold the phases from the previous hop of input samples
    vector<float> lastInputPhases;
    vector<float> lastOutputPhases;
    
    // These containers hold the converted representations from magnitude-phase
    // to magnitude-frequency, used for pitch shifting
    vector<float> analysisMagnitudes;
    vector<float> analysisFrequencies;
    
//    vector<float> synthesisMagnitudes;
    vector<float> synthesisFrequencies;
    float* synthesisMagnitudes;
    float* synthesisPhases;
    
    // Hann window values
    vector<float> analysisWindowBuffer;
    
    vector<float> nextWindowToProcess;
    vector<float> nextCrossOverSampleWindowToProcess;
    
    WindowProcessor windowProcessor;
    

    
    vector<float> binFrequencies;
    
    maxiSample crossOverSample;
    bool hasCrossOverSample;
    
    vector<DelayLine*> amplitudeDelayLines;
    vector<DelayLine*> phaseDelayLines;
};

#endif /* PhaseVocoder_hpp */
