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
    delete crossOverSampleInputBuffer;
    
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
    crossOverSampleInputBuffer = new CircularBuffer(bufferSize);
    hasCrossOverSample = false;
    
    inputBuffer->setSize(bufferSize);
    outputBuffer->setSize(bufferSize);
    outputBuffer->shiftReadPoint(-windowSize);
    crossOverSampleInputBuffer->setSize(bufferSize);
    
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
    crossOverSampleFft = ofxFft::create(windowSize, OF_FFT_WINDOW_HAMMING);
    
    pitchShift = powf(2.0, 12.0 / 12.0);
    pitchCount = 3;
    
    signalFftAmplitudes = new float[windowSize];
    processedFftAmplitudes = new float[windowSize];
    
    binFrequencies.resize(fftSize/2);
    for(int n = 0; n <= fftSize/2; n++) {
        binFrequencies[n] = 2.0 * M_PI * (float)n / (float)fftSize;
    }
    
    std::cout << "begin create delay lines" << std::endl;
    
    amplitudeDelayLines.resize(fftSize/2 + 1);
    phaseDelayLines.resize(fftSize/2 + 1);
    for (int n = 0; n < fftSize / 2 + 1; n++) {
        amplitudeDelayLines[n] = new DelayLine();
        phaseDelayLines[n] = new DelayLine();
//        int delay = ofMap(n, 0, fftSize/2 + 1, 0, 80);
        int delay = ofRandom(0, 300);
        
        amplitudeDelayLines[n]->setup(delay, 44100);
        phaseDelayLines[n]->setup(delay, 44100);
//        if (n < fftSize / 40) {
//            amplitudeDelayLines[n]->setup(0, 44100);
//            phaseDelayLines[n]->setup(0, 44100);
//        } else if (n < fftSize / 20) {
//            amplitudeDelayLines[n]->setup(40, 44100);
//            phaseDelayLines[n]->setup(40, 44100);
//        } else if (n < fftSize / 10) {
//            amplitudeDelayLines[n]->setup(80, 44100);
//            phaseDelayLines[n]->setup(80, 44100);
//        } else {
//            amplitudeDelayLines[n]->setup(160, 44100);
//            phaseDelayLines[n]->setup(160, 44100);
//        }
    }
}

void PhaseVocoder::setCrossOverSample(maxiSample sample) {
    crossOverSample = sample;
    hasCrossOverSample  = true;
}

void PhaseVocoder::addSample(float sample) {
    inputBuffer->push(sample);
    
    if (hasCrossOverSample) {
        float sample = crossOverSample.play();
        crossOverSampleInputBuffer->push(sample);
    }
    
    
    if (inputBuffer->getWriteCount() % hopSize == 0) {
        nextWindowToProcess = inputBuffer->getWindow(windowSize);
        nextCrossOverSampleWindowToProcess = crossOverSampleInputBuffer->getWindow(windowSize);
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
    vector<float> crossOverSampleWindow = nextCrossOverSampleWindowToProcess;
        
    vector<float> outputWindow(windowSize);
    
    // apply window function to signal
    for (int i = 0; i < windowSize; i++) {
        window[i] = window[i] * analysisWindowBuffer[i];
    }

    // execute forward FFT
    fft->setSignal(window);
    float* amplitudes = fft->getAmplitude();
    float* phases = fft->getPhase();
    
//    float* crossOverSampleAmplitudes;
//    float* crossOverSamplePhases;
//    if (hasCrossOverSample) {
//        crossOverSampleFft->setSignal(crossOverSampleWindow);
//        crossOverSampleAmplitudes = crossOverSampleFft->getAmplitude();
//        crossOverSamplePhases = crossOverSampleFft->getPhase();
//    }
//
//    for (int i = 0; i < fftSize / 2; i++) {
//        signalFftAmplitudes[i] = amplitudes[i];
//    }
    
    // DO BLOCK PROCESSING
    
    // BEDIN MULTI-pitch shiftine
    int pitchCount = ofMap(ofGetMouseY(), 0, ofGetHeight(), 20, 1);
    float pitchShiftDistance = ofMap(ofGetMouseX(), 0, ofGetWidth(), 0, 36);
    for (int k = 0; k < pitchCount; k++) {
        float pitchShiftFactor = k * pitchShiftDistance;
        pitchShift = powf(2.0, pitchShiftFactor / 12.0);

        processBlock(amplitudes, phases);

        // execute the reverse FFT
        fft->setPolar(amplitudes, phases);
        float* ifftSignal = fft->getSignal();

        for (int i = 0; i < windowSize / 2; i++) {
            outputWindow[i] += ifftSignal[i];
        }
    }
    for (int i = 0; i < window.size(); i++) {
        // apply the window function befroe writing to output
//        outputWindow[i] = (outputWindow[i]/(float)pitchCount);
    }
    // END MULTI-pitch shiftine
    
//    float pitchShiftFactor = ofMap(ofGetMouseX(), 0, ofGetWidth(), 0, 36);
//    pitchShift = powf(2.0, pitchShiftFactor / 12.0);
    
//    processBlockWithPitchShift(amplitudes, phases);
    
    // Reverse FFT, set to output window
//    fft->setPolar(amplitudes, phases);
//    float* ifftSignal = fft->getSignal();
//    for (int i = 0; i < windowSize / 2; i++) {
//        outputWindow[i] += ifftSignal[i];
//    }

   // write to output buffer
    for (int i = 0; i < window.size(); i++) {
        // apply the window function befroe writing to output
        float windowSample = outputWindow[i] * analysisWindowBuffer[i];

//        float windowSample = outputWindow[i];
        outputBuffer->add(windowSample);
    }

    // shift output buff to next overlapping write point
    outputBuffer->shiftWritePoint(-windowSize + hopSize);
}


// the following code/alogorithm from BelaPlatform:
// https://github.com/BelaPlatform/bela-online-course/tree/master/lectures/lecture-20
void PhaseVocoder::processBlock(float *amplitudes, float *phases) {
    
//    std::cout << "n: " << n << std::endl;
//    amplitudeDelayLines[6]->addSample(amplitudes[0]);
//    phaseDelayLines[7]->addSample(phases[0]);

    for(int n = 0; n <= fftSize/2; n++) {
        // Turn real and imaginary components into amplitude and phase
        float amplitude = amplitudes[n];
        float phase = phases[n];
//
//        std::cout << "Amp in: "<< amplitude << std::endl;

//
        int delayShift = 100;
        float delayMax = ofMap(ofGetMouseX(), 0, ofGetWidth(), -delayShift, delayShift);
        int delay = ofRandom(0, abs(delayMax));
        amplitudeDelayLines[n]->updateDelayTime(delay);
        phaseDelayLines[n]->updateDelayTime(delay);
        amplitude = amplitudeDelayLines[n]->delay(amplitude);
        phase = phaseDelayLines[n]->delay(phase);
//
//        std::cout << "Amp out: "<< amplitude << std::endl;
        
//        if (amplitude < 0.0001) amplitude = 0;
//        cout << amplitude << endl;
//        float amplitude = amplitudes[n];
//        float phase = phases[n];
//
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
    
    float pitchShiftFactor = ofMap(ofGetMouseX(), 0, ofGetWidth(), -24, 24);
    pitchShift = powf(2.0, pitchShiftFactor / 12.0);
    
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


// the following code/alogorithm from BelaPlatform:
// https://github.com/BelaPlatform/bela-online-course/tree/master/lectures/lecture-20
void PhaseVocoder::processBlockWithPitchShift(float *amplitudes, float *phases) {
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
    
//    float pitchShiftFactor = ofMap(ofGetMouseX(), 0, ofGetWidth(), -24, 24);
//    pitchShift = powf(2.0, pitchShiftFactor / 12.0);
    
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



// the following code/alogorithm from BelaPlatform:
// https://github.com/BelaPlatform/bela-online-course/tree/master/lectures/lecture-20
void PhaseVocoder::processBlockWithDelay(float *amplitudes, float *phases) {
    
//    std::cout << "n: " << n << std::endl;
//    amplitudeDelayLines[6]->addSample(amplitudes[0]);
//    phaseDelayLines[7]->addSample(phases[0]);

    for(int n = 0; n <= fftSize/2; n++) {
        // Turn real and imaginary components into amplitude and phase
        float amplitude = amplitudes[n];
        float phase = phases[n];
//
//        std::cout << "Amp in: "<< amplitude << std::endl;

//
        float delayMax = ofMap(ofGetMouseX(), 0, ofGetWidth(), 0, 20);
        int delay = ofRandom(0, delayMax);
        amplitudeDelayLines[n]->updateDelayTime(delay);
        phaseDelayLines[n]->updateDelayTime(delay);
        amplitude = amplitudeDelayLines[n]->delay(amplitude);
        phase = phaseDelayLines[n]->delay(phase);
//
//        std::cout << "Amp out: "<< amplitude << std::endl;
        
//        if (amplitude < 0.0001) amplitude = 0;
//        cout << amplitude << endl;
//        float amplitude = amplitudes[n];
//        float phase = phases[n];
//
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
        int newBin = n;
        
        // Ignore any bins that have shifted above Nyquist
        if (newBin <= fftSize / 2) {
            synthesisMagnitudes[newBin] += analysisMagnitudes[n];
            synthesisFrequencies[newBin] = analysisFrequencies[n];
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

// the following code/alogorithm from BelaPlatform:
// https://github.com/BelaPlatform/bela-online-course/tree/master/lectures/lecture-20
void PhaseVocoder::processBlockWithCrossover(float* amplitudes, float* phases, float* crossOverSampleAmplitudes, float* crossOverSamplePhases) {
    for(int n = 0; n <= fftSize/2; n++) {
        // Turn real and imaginary components into amplitude and phase
        float amplitude = crossOverSampleAmplitudes[n]/5.0 + amplitudes[n] * 2.0;
        float phase = phases[n] + crossOverSamplePhases[n];
        
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
        int newBin = n;//floorf(n * pitchShift + 0.5);

        // Ignore any bins that have shifted above Nyquist
        if (newBin <= fftSize / 2) {
            synthesisMagnitudes[newBin] += analysisMagnitudes[n];
            synthesisFrequencies[newBin] = analysisFrequencies[n];// * pitchShift;
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

