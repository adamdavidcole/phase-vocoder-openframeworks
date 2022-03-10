//
//  PhaseVocoder.cpp
//  sing-to-me
//
//  Created by Adam Cole on 3/10/22.
//

#include "PhaseVocoder.hpp"

void PhaseVocoder::setup(int _windowSize, int _hopSize) {
    windowSize = _windowSize;
    hopSize = _hopSize;

    int bufferSize = 2 * windowSize;
    inputBuffer.setSize(bufferSize);
    outputBuffer.setSize(bufferSize);
    outputBuffer.shiftReadPoint(-windowSize);

    //    fft.setup(fftSize, windowSize, hopSize);
}

void PhaseVocoder::addSample(float sample) {
    inputBuffer.push(sample);
    
    if (inputBuffer.getWriteCount() % hopSize == 0) {
        processWindow();
    }
}

float PhaseVocoder::readSample() {
   return outputBuffer.readAndClear();
}

void PhaseVocoder::processWindow() {
    vector<float> window = inputBuffer.getWindow(windowSize);

    // Apply window function

    // FFT -> block based processing -> IFTT

    // write to output buffer
    for (int i = 0; i < window.size(); i++) {
        float windowSample = window[i];
        outputBuffer.add(windowSample);
    }

    outputBuffer.shiftWritePoint(-windowSize + hopSize);
}
