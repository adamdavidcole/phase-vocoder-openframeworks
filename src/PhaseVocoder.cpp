//
//  PhaseVocoder.cpp
//  sing-to-me
//
//  Created by Adam Cole on 3/10/22.
//

#include "PhaseVocoder.hpp"

void WindowProcessor::setup(PhaseVocoder* _phaseVocoder) {
    phaseVocoder = _phaseVocoder;
}

void WindowProcessor::threadedFunction() {
    phaseVocoder->processWindow();
}

//---------------------------------

PhaseVocoder::~PhaseVocoder() {
    delete inputBuffer;
    delete outputBuffer;
}

void PhaseVocoder::setup(int _fftSize, int _windowSize, int _hopSize) {
    
    fftSize = _fftSize;
    windowSize = _windowSize;
    hopSize = _hopSize;

    int bufferSize = 2 * windowSize;
    
    inputBuffer = new CircularBuffer(bufferSize);
    outputBuffer = new CircularBuffer(bufferSize);
    
    inputBuffer->setSize(bufferSize);
    outputBuffer->setSize(bufferSize);
    outputBuffer->shiftReadPoint(-windowSize);
    
    windowProcessor.setup(this);

    fft.setup(fftSize, windowSize, windowSize);
}

void PhaseVocoder::addSample(float sample) {
    inputBuffer->push(sample);
    
    if (inputBuffer->getWriteCount() % hopSize == 0) {
        nextWindowToProcess = inputBuffer->getWindow(windowSize);
        windowProcessor.threadedFunction();
    }
}

float PhaseVocoder::readSample() {
   return outputBuffer->readAndClear();
}

void PhaseVocoder::processWindow() {
    vector<float> window = nextWindowToProcess;

    // Apply window function

    // FFT ->
    for (int i = 0; i < window.size(); i++) {
        if (fft.process(window[i])) {
            cout << "NEW fft at: " << i << endl;
        }
        
    }
    
    // block based processing ->
    
    // IFTT

    // write to output buffer
    for (int i = 0; i < window.size(); i++) {
        float windowSample = window[i];
        outputBuffer->add(windowSample);
    }

    outputBuffer->shiftWritePoint(-windowSize + hopSize);
}
