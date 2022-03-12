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
    delete[] curFftAmplitudes;
    delete[] real;
    delete[] imaginary;
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
    synthesisFrequencies.resize(fftSize / 2 + 1);
    synthesisMagnitudes = new float[fftSize / 2 + 1];
    synthesisPhases = new float[fftSize / 2 + 1];
    real = new float[fftSize / 2 + 1];
    imaginary = new float[fftSize / 2 + 1];
    
    analysisWindowBuffer.resize(windowSize);
    for (int i = 0; i < windowSize; i++) {
        analysisWindowBuffer[i] =  0.5f * (1.0f - cosf(2.0 * M_PI * i / (float)(windowSize - 1)));
    }
    
    windowProcessor.setup(this);

    fft.setup(fftSize, windowSize, windowSize);
    ifft.setup(fftSize, windowSize, windowSize);
    
    fft2 = ofxFft::create(windowSize, OF_FFT_WINDOW_HAMMING);
    
    calculationsForGui.resize(2);
    
    pitchShift = powf(2.0, 3.0 / 12.0);
    
    indata = new float[windowSize];
    outdata = new float[windowSize];
    for (int i = 0; i < windowSize; i++) {
        indata[i] = 0;
        outdata[i] = 0;
    }
    
    curFftAmplitudes = new float[windowSize];
    
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
    int maxBinIndex = 0; // index of bin with peak magnitude
    float maxBinValue = 0; // magnitude of peak bin
//
//    maxiFFT fft;
//    maxiIFFT ifft;
//
//    fft.setup(fftSize, windowSize, windowSize);
//    ifft.setup(fftSize, windowSize, windowSize);
    
    // buffer of samples to process
    vector<float> window = nextWindowToProcess;
    for (int i = 0; i < windowSize; i++) {
        window[i] = window[i] * analysisWindowBuffer[i];
    }
    
    fft2->setSignal(window);
    float* amplitudes = fft2->getAmplitude();
    float* phases = fft2->getPhase();
    
    for (int i = 0; i < fftSize / 2; i++) {
        curFftAmplitudes[i] = amplitudes[i];
    }
    
    
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
        real[n] = imaginary[n] = 0;
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
        
        // Now convert magnitude and phase back to real and imaginary components
        real[n] = synthesisMagnitudes[n] * cosf(outPhase);
        imaginary[n] = synthesisMagnitudes[n] * sinf(outPhase);
        
        
        amplitudes[n] = synthesisMagnitudes[n];
        phases[n] = outPhase;
        // Also store the complex conjugate in the upper half of the spectrum
//        if(n > 0 && n < gFftSize / 2) {
//            gFft.fdr(gFftSize - n) = gFft.fdr(n);
//            gFft.fdi(gFftSize - n) = -gFft.fdi(n);
//        }
        
        // Save the phase for the next hop
        lastOutputPhases[n] = outPhase;
    }
    
    
    
    fft2->setPolar(amplitudes, phases);
//    fft2->setCartesian(real, imaginary);
    float* ifftSignal = fft2->getSignal();
//    for (int i = 0; i < windowSize; i++) {
//        indata[i] = window[i];
//    }
    
//    smbPitchShift(pitchShift, windowSize, fftSize, hopSize, 44100, indata, outdata);
    
//
  
//    // Apply window function
//
//    // FFT ->
//    for (int i = 0; i < window.size(); i++) {
//        fft.process(window[i]);
//    }
//
//    // block based processing ->
//    float* amplitudes = fft.magnitudes;
//    float* phases = fft.phases;

//    float* synthMag = new float[fftSize];
//    float* synthAmp = new float[fftSize];
//
////
//    for (int n = 0; n < fftSize / 2; n++) {
//        float amplitude = amplitudes[n];
//        float phase = phases[n];
//
//
//        // calculate the phase difference between this hop and the previous one
//        float phaseDiff = phase - lastInputPhases[n];
//
//        float binCenterFrequency = TWO_PI * (float) n / (float) fftSize;
//        phaseDiff = wrapPhase(phaseDiff - binCenterFrequency * hopSize);
//
////        float binDeviation = (phaseDiff / TWO_PI) / (float)hopSize;
////        float binDeviation = phaseDiff / hopSize;
////        float binDeviation = ofMap(phaseDiff, -M_PI, M_PI, -0.5, 0.5);
//        float binDeviation = phaseDiff / (hopSize * TWO_PI);
//
//        analysisFrequencies[n] = (float)n + binDeviation;
//        analysisMagnitudes[n] = amplitude;
//
//        lastInputPhases[n] = phase;
//
//        if (amplitude > maxBinValue) {
//            maxBinValue = amplitude;
//            maxBinIndex = n;
//        }
//
//        calculationsForGui[0] = (float) maxBinIndex;
//        calculationsForGui[1] = analysisFrequencies[maxBinIndex] * 44100 / (float) fftSize;
//    }
////
////    // IFTT
////    // clean synthesis arrays
//    for (int i = 0; i < fftSize; i++) {
//        synthesisMagnitudes[i] = 0;
//        synthesisPhases[i] = 0;
//        synthesisFrequencies[i] = 0;
//    }
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
//    if (pitchShift != 0) {
//        for (int n = 0; n < fftSize / 2; n++) {
//            int newBin = floorf(n * pitchShift + 0.5);
//
//            if (newBin <= fftSize / 2) {
//                synthesisMagnitudes[newBin] += analysisMagnitudes[n];
//                synthesisFrequencies[newBin] += analysisFrequencies[n] * pitchShift;
//            }
//
//            float amplitude = synthesisMagnitudes[n];
//            float binDeviation = synthesisFrequencies[n] - n;
//            float phaseDiff = binDeviation * TWO_PI * (float) hopSize / (float) fftSize;
//
//            float binCenterFrequency = TWO_PI * (float) n / (float) fftSize;
//            phaseDiff += binCenterFrequency * hopSize;
//
//            float outPhase = wrapPhase(lastOutputPhases[n] + phaseDiff);
//            synthesisPhases[n] = outPhase;
//            lastOutputPhases[n] = outPhase;
//
////            if (n > 0 && n < fftSize / 2) {
////                float realComponent = amplitude * cosf(outPhase);
////                float imaginaryComponent = -amplitude * sinf(outPhase);
////
////                synthesisMagnitudes[n + fftSize/2] = amplitude;
////                synthesisPhases[n + fftSize/2] = phases[n];
////            }
//        }
//    } else {
//        for (int i = 0; i < fftSize; i++) {
//            synthesisMagnitudes[i] = analysisMagnitudes[i];
//            synthesisPhases[i] = phases[i];
//        }
//    }
//
//    vector<float> outputWindow(windowSize);
////
//    for (int i = 0; i < fftSize/2; i++) {
//        synthMag[i] = synthesisMagnitudes[i];
//        synthAmp[i] = synthesisPhases[i];
//    }
//
//    vector<float> outputWindow(windowSize);
//    for (int i = 0; i < fftSize/2; i++) {
//        synthMag[i] = fft.magnitudes[i];
//        synthAmp[i] = fft.phases[i];
//    }
//
//    for (int i = 0; i < fftSize; i++) {
////        float sample = ifft.process(synthesisMagnitudes, synthesisPhases);
//        float sample = ifft.process(fft.magnitudes, fft.phases);
////        float sample = ifft.process(fft.magnitudes, fft.phases);
//        outputWindow[i] = sample;
//    }
////
//    // write to output buffer
    for (int i = 0; i < window.size(); i++) {
        float windowSample = ifftSignal[i] * analysisWindowBuffer[i];
        outputBuffer->add(windowSample);
    }
    
//    for (int i = 0; i < window.size(); i++) {
//        outputBuffer->add(indata[i]);
//    }

    outputBuffer->shiftWritePoint(-windowSize + hopSize);
//    
//    delete[] synthAmp;
//    delete[] synthMag;
}

void PhaseVocoder::setPitchShift(float _pitchShift) {
    pitchShift = _pitchShift;
}

