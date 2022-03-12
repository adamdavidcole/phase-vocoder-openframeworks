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
    
    delete[] signalFftAmplitudes;
    delete[] processedFftAmplitudes;
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
    
    synthesisFrequencies.resize(fftSize / 2 + 1);
    synthesisMagnitudes = new float[fftSize / 2 + 1];
    synthesisPhases = new float[fftSize / 2 + 1];

    
    analysisWindowBuffer.resize(windowSize);
    for (int i = 0; i < windowSize; i++) {
        analysisWindowBuffer[i] =  0.5f * (1.0f - cosf(2.0 * M_PI * i / (float)(windowSize - 1)));
    }
    
    windowProcessor.setup(this);

    fft = ofxFft::create(windowSize, OF_FFT_WINDOW_HAMMING);
    
    
    pitchShift = powf(2.0, 3.0 / 12.0);
    
    
    signalFftAmplitudes = new float[windowSize];
    processedFftAmplitudes = new float[windowSize];
    
    binFrequencies.resize(fftSize/2);
    for(int n = 0; n <= fftSize/2; n++) {
        binFrequencies[n] = 2.0 * M_PI * (float)n / (float)fftSize;
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
    // buffer of samples to process
    vector<float> window = nextWindowToProcess;
    // apply window function to signal
    for (int i = 0; i < windowSize; i++) {
        window[i] = window[i] * analysisWindowBuffer[i];
    }
    
    // execute forward FFT
    fft->setSignal(window);
    float* amplitudes = fft->getAmplitude();
    float* phases = fft->getPhase();
    
    for (int i = 0; i < fftSize / 2; i++) {
        signalFftAmplitudes[i] = amplitudes[i];
    }
    
    // DO BLOCK PROCESSING
    processBlock(amplitudes, phases);
    
    for (int i = 0; i < fftSize / 2; i++) {
        processedFftAmplitudes[i] = amplitudes[i];
    }
    
    // execute the reverse FFT
    fft->setPolar(amplitudes, phases);
    float* ifftSignal = fft->getSignal();

   // write to output buffer
    for (int i = 0; i < window.size(); i++) {
        // apply the window function befroe writing to output
        float windowSample = ifftSignal[i] * analysisWindowBuffer[i];
        outputBuffer->add(windowSample);
    }

    // shift output buff to next overlapping write point
    outputBuffer->shiftWritePoint(-windowSize + hopSize);
}

// the following code/alogorithm from BelaPlatform:
// https://github.com/BelaPlatform/bela-online-course/tree/master/lectures/lecture-20
void PhaseVocoder::processBlock(float *amplitudes, float *phases) {
    for(int n = 0; n <= fftSize/2; n++) {
        // Turn real and imaginary components into amplitude and phase
        float amplitude = amplitudes[n];
        float phase = phases[n];
        
        // Calculate the phase difference in this bin between the last
        // hop and this one, which will indirectly give us the exact frequency
        float phaseDiff = phase - lastInputPhases[n];
        
        // Subtract the amount of phase increment we'd expect to see based
        // on the centre frequency of this bin (2*pi*n/gFftSize) for this
        // hop size, then wrap to the range -pi to pi
        phaseDiff = wrapPhase(phaseDiff - binFrequencies[n] * hopSize);
        
        // Find deviation from the centre frequency
        float frequencyDeviation = phaseDiff / (float)hopSize;
        
        // Add the original bin number to get the fractional bin where this partial belongs
        analysisFrequencies[n] = ((float)n * 2.0 * M_PI / (float)fftSize) + frequencyDeviation;
        
        // Save the magnitude for later and for the GUI
        analysisMagnitudes[n] = amplitude;
        
        // Save the phase for next hop
        lastInputPhases[n] = phase;
    }
    
    // Zero out the synthesis bins, ready for new data
    for(int n = 0; n <= fftSize / 2; n++) {
        synthesisMagnitudes[n] = synthesisFrequencies[n] = 0;
    }
    
    // Handle the pitch shift, storing frequencies into new bins
    for(int n = 0; n <= fftSize / 2; n++) {
        int newBin = floorf(n * pitchShift + 0.5);
        
        // Ignore any bins that have shifted above Nyquist
        if (newBin <= fftSize / 2) {
            synthesisMagnitudes[newBin] += analysisMagnitudes[n];
            synthesisFrequencies[newBin] = analysisFrequencies[n] * pitchShift;
        }
    }
    
    for(int n = 0; n <= fftSize / 2; n++) {
        // Get the fractional offset from the bin centre frequency
        float frequencyDeviation = synthesisFrequencies[n] - ((float)n * 2.0 * M_PI / (float)fftSize);
        
        // Multiply to get back to a phase value
        float phaseDiff = frequencyDeviation * (float)hopSize;
        
        // Add the expected phase increment based on the bin centre frequency
        phaseDiff +=  binFrequencies[n] * hopSize;
        
        // Advance the phase from the previous hop
        float outPhase = wrapPhase(lastOutputPhases[n] + phaseDiff);
        
        // Store the updated amplitude/phase
        amplitudes[n] = synthesisMagnitudes[n];
        phases[n] = outPhase;
        
        // Save the phase for the next hop
        lastOutputPhases[n] = outPhase;
    }
}

void PhaseVocoder::setPitchShift(float _pitchShift) {
    pitchShift = _pitchShift;
}

