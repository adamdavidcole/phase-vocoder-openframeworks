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
    glitchAmount = 0;
    glitchIntensity = 0;
    
    delete inputBuffer;
    delete outputBuffer;
    delete crossOverSampleInputBuffer;
    
    delete[] synthesisMagnitudes;
    delete[] synthesisPhases;
    
    delete[] signalFftAmplitudes;
    delete[] processedFftAmplitudes;
    
    delete[] amplitudesOut;
    delete[] phasesOut;
}

void PhaseVocoder::setup(int _fftSize, int _windowSize, int _hopSize) {
    fftSize = _fftSize;
    windowSize = _windowSize;
    hopSize = _hopSize;
    currMode = simplePitchShift;
    
    pitchShift = 1;
    pitchCount = 1;
    

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
    
    binDelays.resize(fftSize/2 + 1);
    for (int n = 0; n < binDelays.size(); n++) {
        binDelays[n] = ofRandom(1.0);
    }
    
    windowProcessor.setup(this);

    fft = ofxFft::create(windowSize);
    ifft = fft;
    crossOverSampleFft = ofxFft::create(windowSize, OF_FFT_WINDOW_HAMMING);
    
   
    
    signalFftAmplitudes = new float[windowSize];
    processedFftAmplitudes = new float[windowSize];
    
    amplitudesOut = new float[fftSize/2 + 1];
    phasesOut = new float[fftSize/2 + 1];
    for (int i = 0; i < fftSize/2 + 1; i++) {
        amplitudesOut[i] = 0;
        phasesOut[i] = 0;
    }
    
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
    
//    int pitchCount = ofMap(glitchAmount, 0, 0.75, 1, 30);
    int pitchCount = ofMap(glitchAmount, 0.4, 1.0, 3, 20);
    bool shouldApplyWindow = (currMode == simplePitchShift && pitchShift != 1) || (currMode == multiPitchShift && pitchCount > 1);
        
    // apply window function to signal
    if (shouldApplyWindow) {
        for (int i = 0; i < windowSize; i++) {
            window[i] = window[i] * analysisWindowBuffer[i];
        }
    }

    // execute forward FFT
    fft->setSignal(window);
    float* amplitudes = fft->getAmplitude();
    float* phases = fft->getPhase();
    
//    // for visualization
//    for (int i = 0; i < fftSize / 2; i++) {
//        signalFftAmplitudes[i] = amplitudes[i];
//    }
    
    
    // DO BLOCK PROCESSING
    if (currMode == simplePitchShift) {
//        if (pitchShift == 1) {
//            for (int n = 0; n < fftSize / 2 + 1; n++) {
//                amplitudesOut[n] = amplitudes[n];
//                phasesOut[n] = phases[n];
//            }
//        } else {
//            processBlockWithPitchShift(amplitudes, phases, amplitudesOut, phasesOut);
//        }
        processBlockWithPitchShift(amplitudes, phases, amplitudesOut, phasesOut);

        
        // Reverse FFT, set to output window
        ifft->setPolar(amplitudesOut, phasesOut);
        float* ifftSignal = ifft->getSignal();
        for (int i = 0; i < windowSize / 2; i++) {
            outputWindow[i] += ifftSignal[i];
        }
    } else if (currMode == multiPitchShift) {
        // BEDIN MULTI-pitch shifting
        int pitchCountStart = 0;
        int pitchCountEnd = pitchCount;
        bool includeNegativePitches = false;
        if (glitchAmount > 0.75 && ofRandom(1.5) < glitchAmount) {
            includeNegativePitches = true;
            pitchCountStart = ofMap(glitchAmount, 0.75, 1, 3, -8);
            pitchCountEnd = ofMap(glitchAmount, 0.5, 1, pitchCountStart + 1, 10);
        }
    
        if (pitchCount > 1) {
            for (int k = pitchCountStart; k < pitchCountEnd; k++) {
                float pitchShiftDistance = ofMap(glitchIntensity, 0, 1, 1, 3);
                int pitchShiftFactor = k * pitchShiftDistance;
                pitchShift = powf(2.0, k / 12.0);
    
                processBlockWithPitchShift(amplitudes, phases, amplitudesOut, phasesOut);
    
                // execute the reverse FFT
                fft->setPolar(amplitudesOut, phasesOut);
                float* ifftSignal = fft->getSignal();
                for (int i = 0; i < windowSize / 2; i++) {
                    outputWindow[i] += ifftSignal[i];
                }
            }
        } else {
            for (int i = 0; i < windowSize / 2; i++) {
                outputWindow[i] += window[i];
            }
        }
        // END MULTI-pitch shifting
    } else if (currMode == delaySpectrum) {
        // BEGIN DELAY
        int delayIterations = ofMap(glitchAmount, 0, 1, 1, 5);
        for (int i = 0; i < delayIterations; i++) {
            processBlockWithDelay(amplitudes, phases, amplitudesOut, phasesOut);
            
            // Reverse FFT, set to output window
            fft->setPolar(amplitudesOut, phasesOut);
            float* ifftSignal = fft->getSignal();
            for (int i = 0; i < windowSize / 2; i++) {
                outputWindow[i] += ifftSignal[i];
            }
        }
        // END DELAY
    } else if (currMode == crossOverSpectrum) {
        // TODO: crossover mode
        //    float* crossOverSampleAmplitudes;
        //    float* crossOverSamplePhases;
        //    if (hasCrossOverSample) {
        //        crossOverSampleFft->setSignal(crossOverSampleWindow);
        //        crossOverSampleAmplitudes = crossOverSampleFft->getAmplitude();
        //        crossOverSamplePhases = crossOverSampleFft->getPhase();
        //    }
    }
    
    // for visualization
//    for (int i = 0; i < fftSize / 2; i++) {
//        processedFftAmplitudes[i] = amplitudesOut[i];
//    }
//    
   

   // write to output buffer
    for (int i = 0; i < window.size(); i++) {
        // apply the window function befroe writing to output
        float windowSample = outputWindow[i];// * analysisWindowBuffer[i];
        if (shouldApplyWindow) {
            windowSample *= analysisWindowBuffer[i];
        }
        
        outputBuffer->add(windowSample);
    }

    // shift output buff to next overlapping write point
    outputBuffer->shiftWritePoint(-windowSize + hopSize);
}


// the following code/alogorithm from BelaPlatform:
// https://github.com/BelaPlatform/bela-online-course/tree/master/lectures/lecture-20
void PhaseVocoder::processBlock(float *amplitudes, float *phases, float* amplitudesOut, float* phasesOut) {
    
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
//        float delayMax = ofMap(ofGetMouseX(), 0, ofGetWidth(), -delayShift, delayShift);
//        int delay = ofRandom(0, abs(delayMax));
        float delayMax = ofMap(glitchIntensity, 0, 1, -delayShift, delayShift);
        int delay = ofRandom(0, delayMax);
        
//        delay = 0;
        
//        amplitudeDelayLines[n]->updateDelayTime(delay);
//        phaseDelayLines[n]->updateDelayTime(delay);
//        amplitude = amplitudeDelayLines[n]->delay(amplitude);
//        phase = phaseDelayLines[n]->delay(phase);
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
//
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
        amplitudesOut[n] = synthesisMagnitudes[n];
        phasesOut[n] = outPhase;
        
        // Save the phase for the next hop
        lastOutputPhases[n] = outPhase;
    }
}


// the following code/alogorithm from BelaPlatform:
// https://github.com/BelaPlatform/bela-online-course/tree/master/lectures/lecture-20
void PhaseVocoder::processBlockWithPitchShift(float *amplitudes, float *phases, float* amplitudesOut, float* phasesOut) {
    for(int n = 0; n <= fftSize/2 + 1; n++) {
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
    for(int n = 0; n <= fftSize / 2 + 1; n++) {
        synthesisMagnitudes[n] = synthesisFrequencies[n] = 0;
        amplitudesOut[n] = phasesOut[n] = 0;
    }
    
    
    // Handle the pitch shift, storing frequencies into new bins
    for(int n = 0; n <= fftSize / 2 + 1; n++) {
        int newBin = floorf(n * pitchShift + 0.5);
        
        
        // Ignore any bins that have shifted above Nyquist
        if (newBin <= fftSize / 2) {
            synthesisMagnitudes[newBin] += analysisMagnitudes[n];
            synthesisFrequencies[newBin] = analysisFrequencies[n] * pitchShift;
        }
    }
    
    for(int n = 0; n <= fftSize / 2 + 1; n++) {
        // Get the fractional offset from the bin centre frequency
        float frequencyDeviation = synthesisFrequencies[n] - ((float)n * 2.0 * M_PI / (float)fftSize);
        
        // Multiply to get back to a phase value
        float phaseDiff = frequencyDeviation * (float)hopSize;
        
        // Add the expected phase increment based on the bin centre frequency
        phaseDiff +=  binFrequencies[n] * hopSize;
        
        // Advance the phase from the previous hop
        float outPhase = wrapPhase(lastOutputPhases[n] + phaseDiff);
        
        // Store the updated amplitude/phase
        amplitudesOut[n] = synthesisMagnitudes[n];
        phasesOut[n] = outPhase;
        
        // Save the phase for the next hop
        lastOutputPhases[n] = outPhase;
    }
}



// the following code/alogorithm from BelaPlatform:
// https://github.com/BelaPlatform/bela-online-course/tree/master/lectures/lecture-20
void PhaseVocoder::processBlockWithDelay(float *amplitudes, float *phases, float* amplitudesOut, float* phasesOut) {
    
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
//        float delayMax = ofMap(ofGetMouseX(), 0, ofGetWidth(), 0, 20);
//        int delay = ofRandom(0, delayMax);
        int delayShift = 100;
        float delayMax = ofMap(glitchAmount, 0, 1, 0, delayShift);

        int delay = binDelays[n] * delayMax;

        amplitudeDelayLines[n]->updateDelayTime(delay);
        phaseDelayLines[n]->updateDelayTime(delay);
        amplitude = amplitudeDelayLines[n]->delay(amplitude);
        phase = phaseDelayLines[n]->delay(phase);

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
        amplitudesOut[n] = synthesisMagnitudes[n];
        phasesOut[n] = outPhase;
        
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
    if (pitchShift == _pitchShift) return;
    
    pitchShift = _pitchShift;
    for (int n = 0; n < fftSize/2 + 1; n++) {
        lastInputPhases[n] = 0;
        lastOutputPhases[n] = 0;
    }
}

void PhaseVocoder::setRandomPitchShift() {
   if (currMode != simplePitchShift) return;
    
   float maxShift = sin(ofGetElapsedTimeMillis()/1000) * ofMap(glitchIntensity, 0, 1, 0, 8);
   float shiftStart = sin(ofGetElapsedTimeMillis()/1000) * ofMap(glitchAmount, 0, 1, 0, 8);
   float pitchShiftFactor = shiftStart + maxShift;
    
   pitchShift = powf(2.0, pitchShiftFactor / 12.0);
}

void PhaseVocoder::setMode(PhaseVocoderMode mode) {
    currMode = mode;
}

void PhaseVocoder::setRandomMode() {
    float rand = ofRandom(1.0);
    if (rand < 0.33) {
        currMode = multiPitchShift;
    } else if (rand < 0.66) {
        currMode = multiPitchShift;
    } else {
        currMode = delaySpectrum;
    }
}
