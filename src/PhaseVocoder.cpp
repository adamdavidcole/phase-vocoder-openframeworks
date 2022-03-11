//
//  PhaseVocoder.cpp
//  sing-to-me
//
//  Created by Adam Cole on 3/10/22.
//

#include "PhaseVocoder.hpp"
#include "smpPitchShift.hpp"

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
    delete[] synthesisMagnitudes;
    delete[] synthesisPhases;
    
    delete[] indata;
    delete[] outdata;
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
    lastOutputPhases.resize(fftSize);
    analysisMagnitudes.resize(fftSize / 2 + 1);
    analysisFrequencies.resize(fftSize / 2 + 1);
    
//    synthesisMagnitudes.resize(fftSize / 2 + 1);
    synthesisFrequencies.resize(fftSize);
    synthesisMagnitudes = new float[fftSize];
    synthesisPhases = new float[fftSize];
    
    analysisWindowBuffer.resize(windowSize);
    for (int i = 0; i < windowSize; i++) {
        analysisWindowBuffer[i] =  0.5f * (1.0f - cosf(2.0 * M_PI * i / (float)(windowSize - 1)));
    }
    
    windowProcessor.setup(this);

    fft.setup(fftSize, windowSize, windowSize);
    ifft.setup(fftSize, windowSize, windowSize);
    
    calculationsForGui.resize(2);
    
    pitchShift = 0.5;
    
    indata = new float[windowSize];
    outdata = new float[windowSize];
    for (int i = 0; i < windowSize; i++) {
        indata[i] = 0;
        outdata[i] = 0;
    }
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
//    maxiFFT fft;
//    maxiIFFT ifft;
//
//    fft.setup(fftSize, windowSize, windowSize);
//    ifft.setup(fftSize, windowSize, windowSize);
    
    // buffer of samples to process
    vector<float> window = nextWindowToProcess;
    for (int i = 0; i < windowSize; i++) {
        indata[i] = window[i];
    }
    
//    smbPitchShift(pitchShift, windowSize, fftSize, hopSize, 44100, indata, outdata);
    
//
    int maxBinIndex = 0; // index of bin with peak magnitude
    float maxBinValue = 0; // magnitude of peak bin
//
//    // Apply window function
//
//    // FFT ->
    for (int i = 0; i < window.size(); i++) {
        fft.process(window[i] * analysisWindowBuffer[i]);
    }

    // block based processing ->
    float* amplitudes = fft.magnitudes;
    float* phases = fft.phases;
    
    float* synthMag = new float[fftSize];
    float* synthAmp = new float[fftSize];
    
//
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
//
//    // IFTT
//    // clean synthesis arrays
    for (int i = 0; i < fftSize; i++) {
        synthesisMagnitudes[i] = 0;
        synthesisPhases[i] = 0;
        synthesisFrequencies[i] = 0;
    }
//
//    // robotization
////    for (int i = 0; i < fftSize; i++) {
////        synthesisMagnitudes[i] = fft.magnitudes[i];
////        synthesisPhases[i] = 0;
////
////    }
////
////    // whisperization
////    for (int i = 0; i < fftSize; i++) {
////        synthesisMagnitudes[i] = fft.magnitudes[i];
////        synthesisPhases[i] = ofRandom(0, TWO_PI);
////    }
//
    // pitch shift
    if (pitchShift != 0) {
        for (int n = 0; n < fftSize / 2; n++) {
            int newBin = floorf(n * pitchShift + 0.5);

            if (newBin <= fftSize / 2) {
                synthesisMagnitudes[newBin] += analysisMagnitudes[n];
                synthesisFrequencies[newBin] += analysisFrequencies[n] * pitchShift;
            }

            float amplitude = synthesisMagnitudes[n];
            float binDeviation = synthesisFrequencies[n] - n;
            float phaseDiff = binDeviation * TWO_PI * (float) hopSize / (float) fftSize;

            float binCenterFrequency = TWO_PI * (float) n / (float) fftSize;
            phaseDiff += binCenterFrequency * hopSize;

            float outPhase = wrapPhase(lastOutputPhases[n] + phaseDiff);
            synthesisPhases[n] = outPhase;
            lastOutputPhases[n] = outPhase;

//            if (n > 0 && n < fftSize / 2) {
//                float realComponent = amplitude * cosf(outPhase);
//                float imaginaryComponent = -amplitude * sinf(outPhase);
//
//                synthesisMagnitudes[n + fftSize/2] = amplitude;
//                synthesisPhases[n + fftSize/2] = phases[n];
//            }
        }
    } else {
        for (int i = 0; i < fftSize; i++) {
            synthesisMagnitudes[i] = analysisMagnitudes[i];
            synthesisPhases[i] = phases[i];
        }
    }
//
    vector<float> outputWindow(windowSize);
//
    for (int i = 0; i < fftSize/2; i++) {
        synthMag[i] = synthesisMagnitudes[i];
        synthAmp[i] = synthesisPhases[i];
    }
  
    
    for (int i = 0; i < fftSize; i++) {
//        float sample = ifft.process(synthesisMagnitudes, synthesisPhases);
        float sample = ifft.process(synthMag, synthAmp);
//        float sample = ifft.process(fft.magnitudes, fft.phases);
        outputWindow[i] = sample;
    }
//
//    // write to output buffer
    for (int i = 0; i < window.size(); i++) {
        float windowSample = outputWindow[i] * analysisWindowBuffer[i];
        outputBuffer->add(windowSample);
    }
    
//    for (int i = 0; i < window.size(); i++) {
//        outputBuffer->add(indata[i]);
//    }

    outputBuffer->shiftWritePoint(-windowSize + hopSize);
    
    delete[] synthAmp;
    delete[] synthMag;
}

void PhaseVocoder::setPitchShift(float _pitchShift) {
    pitchShift = _pitchShift;
}

