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
    
    lastInputPhases.resize(fftSize);
    analysisMagnitudes.resize(fftSize / 2 + 1);
    analysisFrequencies.resize(fftSize / 2 + 1);
    
    analysisWindowBuffer.resize(windowSize);
    for (int i = 0; i < windowSize; i++) {
        analysisWindowBuffer[i] =  0.5f * (1.0f - cosf(2.0 * M_PI * i / (float)(windowSize - 1)));
    }
    
    windowProcessor.setup(this);

    fft.setup(fftSize, windowSize, windowSize);
    
    calculationsForGui.resize(2);
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

float wrapPhase(float phaseIn) {
    if (phaseIn >= 0)
        return fmodf(phaseIn + M_PI, 2.0 * M_PI) - M_PI;
    else
        return fmodf(phaseIn - M_PI, -2.0 * M_PI) + M_PI;
}

void PhaseVocoder::processWindow() {
    // buffer of samples to process
    vector<float> window = nextWindowToProcess;
    
    
    int maxBinIndex = 0; // index of bin with peak magnitude
    float maxBinValue = 0; // magnitude of peak bin

    // Apply window function

    // FFT ->
    for (int i = 0; i < window.size(); i++) {
        fft.process(window[i] * analysisWindowBuffer[i]);
    }
    
    float* amplitudes = fft.magnitudes;
    float* phases = fft.phases;
    
    for (int n = 0; n < fftSize / 2; n++) {
        float amplitude = amplitudes[n];
        float phase = phases[n];
        
        
        // calculate the phase difference between this hop and the previous one
        float phaseDiff = phase - lastInputPhases[n];
    
        float binCenterFrequency = TWO_PI * (float) n / (float) fftSize;
        phaseDiff = wrapPhase(phaseDiff - binCenterFrequency * hopSize);
        
//        float binDeviation = (phaseDiff / TWO_PI) / (float)hopSize;
//        float binDeviation = phaseDiff / hopSize;
//        float binDeviation = ofMap(phaseDiff, -M_PI, M_PI, -0.5, 0.5);
        float binDeviation = phaseDiff / (hopSize * TWO_PI);

        analysisFrequencies[n] = (float)n + binDeviation;
        analysisMagnitudes[n] = amplitude;
        
        lastInputPhases[n] = phase;
        
        if (amplitude > maxBinValue) {
            maxBinValue = amplitude;
            maxBinIndex = n;
        }
        
        calculationsForGui[0] = (float) maxBinIndex;
        calculationsForGui[1] = analysisFrequencies[maxBinIndex] * 44100 / (float) fftSize;
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
