#include "ofApp.h"

#define DISABLE_AUDIO false
#define DISABLE_VIDEO false

float mtofArray[] = {0, 8.661957, 9.177024, 9.722718, 10.3, 10.913383, 11.562325, 12.25, 12.978271, 13.75, 14.567617, 15.433853, 16.351599, 17.323914, 18.354048, 19.445436, 20.601723, 21.826765, 23.124651, 24.5, 25.956543, 27.5, 29.135235, 30.867706, 32.703197, 34.647827, 36.708096, 38.890873, 41.203445, 43.65353, 46.249302, 49., 51.913086, 55., 58.27047, 61.735413, 65.406395, 69.295654, 73.416191, 77.781746, 82.406891, 87.30706, 92.498604, 97.998856, 103.826172, 110., 116.540939, 123.470825, 130.81279, 138.591309, 146.832382, 155.563492, 164.813782, 174.61412, 184.997208, 195.997711, 207.652344, 220., 233.081879, 246.94165, 261.62558, 277.182617,293.664764, 311.126984, 329.627563, 349.228241, 369.994415, 391.995422, 415.304688, 440., 466.163757, 493.883301, 523.25116, 554.365234, 587.329529, 622.253967, 659.255127, 698.456482, 739.988831, 783.990845, 830.609375, 880., 932.327515, 987.766602, 1046.502319, 1108.730469, 1174.659058, 1244.507935, 1318.510254, 1396.912964, 1479.977661, 1567.981689, 1661.21875, 1760., 1864.655029, 1975.533203, 2093.004639, 2217.460938, 2349.318115, 2489.015869, 2637.020508, 2793.825928, 2959.955322, 3135.963379, 3322.4375, 3520., 3729.31, 3951.066406, 4186.009277, 4434.921875, 4698.63623, 4978.031738, 5274.041016, 5587.651855, 5919.910645, 6271.926758, 6644.875, 7040., 7458.620117, 7902.132812, 8372.018555, 8869.84375, 9397.272461, 9956.063477, 10548.082031, 11175.303711, 11839.821289, 12543.853516, 13289.75};

void printFloatVector(vector<float> vec) {
    std::cout << "[ ";
    for (float i : vec) {
        std::cout << i << ' ';
    }
    std::cout << "]" << endl;
}

float mapSquared(float value, float start1, float stop1, float start2, float stop2) {
  float inT = ofMap(value, start1, stop1, 0, 1);
  float outT = inT * inT;
  return ofMap(outT, 0, 1, start2, stop2);
}

//--------------------------------------------------------------
void ofApp::setup() {
    currState = AppState::IDLE;
    phaseStartTime = -1;
    
    glitchIntensity = 0;
    glitchAmount = 0;
    feedbackAmount = 0;
    pianoSamp.load(ofToDataPath("piano-chrom.wav"));
//    pianoSamp.load(ofToDataPath("test-sound.wav"));
//    pianoSamp.load(ofToDataPath("test-sound.wav"));
//    pianoSamp.load(ofToDataPath("whale-sound1.wav"));

    bufferSize = 512;
    sampleRate = 44100;
    
    frequency = 430.66;
    
    ofxMaxiSettings::setup(sampleRate, 2, bufferSize);
    
    fftSize = 1024;
    fftBins = fftSize / 2;
    binFreq = sampleRate / fftSize;
    
    windowSize = fftSize;
    hopSize = windowSize / 4;
    
    phaseVocoder.setup(fftSize, windowSize, hopSize);
//    phaseVocoder.setCrossOverSample(pianoSamp);
//
//    constQ.resize(128);
//    chroma.resize(12);

    if (!DISABLE_AUDIO) {
        ofSoundStreamSetup(2, 1, this, sampleRate, bufferSize, 4);
    }
    
    recordedSamplesSize = sampleRate * 5;
    recordedSamplesCount = 0;
    recordedSamplesReadPoint = 0;
    recordedSamples.resize(recordedSamplesSize);
    isRecording = false;
    hasCleanedRecording = false;
    
    channelMix = 0.5;
    
    //// VISUALS
    
    camWidth = 1280;
    camHeight = 720;
    
    if (!DISABLE_VIDEO) {
        webcam.setup(camWidth, camHeight);
    
        pixelsToDraw.allocate(camWidth, camHeight, 3);
        myTexture.allocate (camWidth, camHeight, GL_RGB);
        
        float planeScale = 1;
        planeWidth = camWidth * planeScale;
        planeHeight = camHeight * planeScale;
        planeX = 0;
        planeY = 0;
        
        int planeGridSize = 20;
        int planeColumns = planeWidth / planeGridSize;
        int planeRows = planeHeight / planeGridSize;

        plane.set(planeWidth,
                  planeHeight,
                  planeColumns,
                  planeRows,
                  OF_PRIMITIVE_TRIANGLES);
        plane.mapTexCoordsFromTexture(myTexture);

        feedbackImg.allocate(planeWidth, planeHeight, OF_IMAGE_COLOR);
        shouldClearFeedbackImg = false;

        shader.load("shader");
    }
}

//--------------------------------------------------------------
void ofApp::update() {
    if (!DISABLE_VIDEO) {
        webcam.update();
        
        // If the grabber indeed has fresh data,
        if(webcam.isFrameNew()){

            ofPixels image = webcam.getPixels();
            
            for (int y = 0; y < camHeight; y++) {
                for (int x = 0; x < camWidth; x++) {
                    int shift = ofMap(mouseX, 0, ofGetWidth(), -200, 200);
                    ofColor currColor = image.getColor(x, y);
                    pixelsToDraw.setColor(x, y, currColor);
                }
            }
            
            if (shouldClearFeedbackImg) {
                // hack for wierd inverse x position bug
                for (int x = 0; x < feedbackImg.getWidth(); x++) {
                   for (int y = 0; y < feedbackImg.getHeight(); y++) {
                       ofColor camColor = image.getColor(camWidth - x, y);
                       feedbackImg.setColor(x, y, camColor);
                   }
               }
                
                feedbackImg.update();
            }

            myTexture.loadData(pixelsToDraw);
        }
        
        if (!shouldClearFeedbackImg) {
            int x = planeX;
            int y = planeY;
            feedbackImg.grabScreen(x, y, planeWidth, planeHeight);
        }
    }
    
    float ellapsedTime = ofGetElapsedTimef() - phaseStartTime;
    if (isRunningPhases()) {
        cout << "running phases ellapsed time: " << to_string(ellapsedTime)  << endl;
        if (ellapsedTime < 5) {
            // normal repititon
            currState = AppState::RUNNING_PHASE_ONE;
        } else if (ellapsedTime < 15) {
            // slow oscillating degradation
            currState = AppState::RUNNING_PHASE_TWO;
        } else if (ellapsedTime < 45) {
            // rise to full glitchiness
            currState = AppState::RUNNING_PHASE_THREE;
        } else if (ellapsedTime < 45) {
            currState = AppState::RUNNING_PHASE_FOUR;
        } else if (ellapsedTime < 55) {
            currState = AppState::RUNNING_PHASE_FIVE;
        } else if (ellapsedTime < 60) {
            currState = AppState::RUNNING_PHASE_SIX;
        } else {
            // TODO: reset state
            currState = AppState::IDLE;
        }
        
//        setGlitchAmount(0);
    } else {
//        glitchAmount = 0;
//        glitchIntensity = 0;
//        phaseVocoder.glitchAmount = 0;
//        phaseVocoder.glitchIntensity = 0;
    }
    
    switch (currState) {
        case AppState::IDLE:
            break;
        case AppState::RECORDING:
            break;
        case AppState::RUNNING_PHASE_ONE:
            glitchAmount = 0;
            glitchIntensity = 0;
            break;
        case AppState::RUNNING_PHASE_TWO:
        {
            float phaseTwoStart = 5;
            float phaseTwoDuration = 10;
            float phaseTwoMaxDistortion = 0.4;
            float progress = ofMap(ellapsedTime, phaseTwoStart, phaseTwoStart + phaseTwoDuration, 0, 1);

            float phase = ofMap(sin(ellapsedTime* 5.0 * progress), -1, 1, 0, phaseTwoMaxDistortion) * (progress + 0.25);

            glitchAmount = phase;
            glitchIntensity = phase;
            
            phaseVocoder.glitchAmount = glitchAmount;
            phaseVocoder.glitchIntensity = glitchIntensity;
            
            float pitchShiftDegree = floor(ofMap(glitchAmount, 0, phaseTwoMaxDistortion, 0, 12));
            if (ofRandom(1.0) > 0.5) pitchShiftDegree*= -1;
            float pitchShfit = powf(2.0, (int)pitchShiftDegree / 12.0);

            phaseVocoder.setMode(simplePitchShift);
            phaseVocoder.setPitchShift(pitchShfit);
        }
            break;
        case AppState::RUNNING_PHASE_THREE:
        {
            float phaseThreeStart = 15;
            float phaseThreeDuration = 30;
            float phaseThreeMinDistortion = 0.4;
            float phaseThreeMaxDistortion = 1.0;
            float progress = ofMap(ellapsedTime, phaseThreeStart, phaseThreeStart + phaseThreeDuration, 0, 1);
            
            float phase = ofMap(sin(ellapsedTime* 5.0 * progress), -1, 1, phaseThreeMinDistortion, phaseThreeMaxDistortion) * (progress + 0.25);

            glitchAmount = phase;
            glitchIntensity = phase;
            
            phaseVocoder.glitchAmount = glitchAmount;
            phaseVocoder.glitchIntensity = glitchIntensity;
            

            float randomVocoderMore = ofRandom(1.0);
            if (randomVocoderMore < 0.2) {
                float pitchShiftDegree = floor(ofMap(glitchAmount, phaseThreeMinDistortion, phaseThreeMaxDistortion, 6, 18));
                if (ofRandom(1.0) > 0.5) {pitchShiftDegree*= -1;}
                pitchShiftDegree = ofClamp(pitchShiftDegree, -6, 18);
                float pitchShfit = powf(2.0, (int)pitchShiftDegree / 12.0);
            } else {
                phaseVocoder.setMode(multiPitchShift);
            }
            
            float randomSampleSkipVal = ofRandom(1.0);
            if (randomSampleSkipVal < 0.03 * progress) {
                randomSampleSkip();
            }
        }
             break;
//        case AppState::RUNNING_PHASE_FOUR:
//        {
//            float phaseFourStart = 30;
//            float phaseFourDuration = 45;
//            float phaseFourMinDistortion = 0.8;
//            float phaseFourMaxDistortion = 1.0;
//            float progress = ofMap(ellapsedTime, phaseFourStart, phaseFourStart + phaseFourDuration, 0, 1);
//
//            float phase = ofMap(sin(ellapsedTime* 5.0 * progress), -1, 1, phaseFourMinDistortion, phaseFourMaxDistortion) * (progress + 0.25);
////
////            float phase = ofMap(sin(ellapsedTime*5), -1, 1, phaseFourMinDistortion, phaseFourMaxDistortion) * progress;
////
//            glitchAmount = phase;
//            glitchIntensity = mapSquared(glitchAmount, phaseFourMinDistortion, phaseFourMaxDistortion, phaseFourMinDistortion, phaseFourMaxDistortion);
//
//            float randomVocoderMore = ofRandom(1.0);
//            if (randomVocoderMore < 0.2) {
//                float pitchShiftDegree = floor(ofMap(glitchAmount, phaseFourMinDistortion, phaseFourMaxDistortion, 3, 24));
//                if (ofRandom(1.0) > 0.5) {pitchShiftDegree*= -1;}
//                pitchShiftDegree = ofClamp(pitchShiftDegree, -6, 18);
//                float pitchShfit = powf(2.0, (int)pitchShiftDegree / 12.0);
//            } else if (randomVocoderMore < 0.6) {
//                phaseVocoder.setMode(multiPitchShift);
//            } else {
//                phaseVocoder.setMode(delaySpectrum);
//            }
            
//            float randomSampleSkipVal = ofRandom(1.0);
//            if (randomSampleSkipVal < 0.02 + progress * 0.02) {
//                randomSampleSkip();
//            }
//        }
//             break;
//        case AppState::RUNNING_PHASE_FIVE:
//        {
//            float phaseFiveStart = 45;
//            float phaseFiveDuration = 55;
//            float phaseFiveMinDistortion = 1.8;
//            float phaseFiveMaxDistortion = 2.2;
//
//            float progress = ofMap(ellapsedTime - phaseFiveStart, 0, phaseFiveDuration, 0, 1);
//
//            float phase = ofMap(sin(ellapsedTime*10), -1, 1, phaseFiveMinDistortion, phaseFiveMaxDistortion) * progress;
//
//            glitchAmount = phase;
//            glitchIntensity = mapSquared(glitchAmount, phaseFiveMinDistortion, phaseFiveMaxDistortion, phaseFiveMinDistortion, phaseFiveMaxDistortion);
//        }
//             break;
        case AppState::RUNNING_PHASE_SIX:
            glitchAmount = 0;
            glitchIntensity = 0;
            break;
    }
    
//    uint64_t ellapsedTime = ofGetElapsedTimef();
//    float phase = sin(ellapsedTime) * ofMap(ellapsedTime, 0, 60, 0, 1);
//    glitchAmount = ofMap(phase, -1, 1, 0, 2);
//    glitchIntensity = mapSquared(glitchAmount, 0, 2, 0, 2);


    
    // CHROMAGRAM!
//    int currentBin = 0;
//    int fftBins = fftSize / 2;
//    for (int i = 0; i < constQ.size(); i++) {
//        constQ[i] = 0;
//
//        for (int n = currentBin; n < fftBins; n++) {
//            float handler =(mtofArray[i+1] - mtofArray[i])/2.f;
//            if (binFreq * n > mtofArray[i] - handler && binFreq * n < mtofArray[i+1] - handler) {
//                constQ[i] += fft.magnitudes[n];
//                currentBin++;
//            }
//        }
//    }
//
//    for (int i = 0; i < chroma.size(); i++) {
//        chroma[i] = 0;
//    }
//
//    int j = 0;
//    for (int i = 0; i < constQ.size(); i++) {
//        chroma[j] += sqrt(constQ[i]);
//        j++;
//        if (j == 12) j = 0;
//    }
//
//    int maxChroma = -1;
//    int maxChromaIndex = 0;
//    for (int i = 0; i < chroma.size(); i++) {
//        if (chroma[i] > maxChroma) {
//            maxChroma = chroma[i];
//            maxChromaIndex = i;
//        }
//    }
    
//    feedbackAmount = ofMap(sin(ofGetElapsedTimeMillis()/10000.0), -1, 1, 0, 0.9);
}

//--------------------------------------------------------------
void ofApp::draw() {
    if (!DISABLE_VIDEO) {
        shader.begin();
        shader.setUniform2f("u_res", plane.getWidth(), plane.getHeight());
        shader.setUniformTexture("inputTexture", myTexture, 1);
        shader.setUniformTexture("feedbackTexture", feedbackImg, 2);
        shader.setUniform1f("glitchIntensity", glitchIntensity);
        shader.setUniform1f("glitchAmount", glitchAmount);
        shader.setUniform1f("feedbackAmount", feedbackAmount);
        
        ofPushMatrix();
        ofTranslate(planeWidth/2, planeHeight/2);
        ofRotateDeg(180);
        
        plane.draw();
        
        ofPopMatrix();

        shader.end();
        
        int margin = 20;
        feedbackImg.draw(0, planeHeight + margin);
    }
    
    string gfactorString = "Glitch amount: " + to_string(glitchAmount);
    string gfactorString2 = "Glitch intensity: " + to_string(glitchIntensity);
    string gfactorString3 = "Feedback amount: " + to_string(feedbackAmount);
    int currStateInt = (int)currState;
    string currstateString = "CurrState: " + to_string(currStateInt);
    string currPitch = "Pitch shift: " + to_string(phaseVocoder.pitchShift);

    ofSetColor(255, 255, 255);
    ofDrawBitmapString(gfactorString, 50, 50);
    ofDrawBitmapString(gfactorString2, 50, 65);
    ofDrawBitmapString(gfactorString3, 50, 80);
    ofDrawBitmapString(currstateString, 50, 95);
    ofDrawBitmapString(currPitch, 50, 110);

//    int fft2Bins = 200;
//    float bandWidth = ofGetWidth()/ (float)fft2Bins;
//    float* signalMagnitudes = phaseVocoder.signalFftAmplitudes;
//    float* processedMagnitudes = phaseVocoder.processedFftAmplitudes;
//
//    ofSetColor(255, 255, 255, 255/2);
//    for (int i = 0; i < fft2Bins; i++) {
//        float fftMagnitude = signalMagnitudes[i] * 20000;
//        float x = i * bandWidth;
//        ofDrawRectangle(x, ofGetHeight() / 2, bandWidth, -fftMagnitude);
//    }
//
//    ofSetColor(30, 30, 255, 255/2);
//    for (int i = 0; i < fft2Bins; i++) {
//        float fftMagnitude = processedMagnitudes[i] * 20000;
//        float x = i * bandWidth;
//        ofDrawRectangle(x, ofGetHeight() / 2, bandWidth, -fftMagnitude);
//    }
    
//    ofDrawBitmapString("Bin: " + to_string(phaseVocoder.calculationsForGui[0]), 100, 100);
//    ofDrawBitmapString("Oscillator frequency: " + to_string(frequency), 100, 120);
//    ofDrawBitmapString("Calculated frequency: " + to_string(phaseVocoder.calculationsForGui[1]), 100, 140);
//    ofDrawBitmapString("Error: " + to_string(abs(phaseVocoder.calculationsForGui[1] - frequency)), 100, 160);

//    CHROMAGRAM
//    float chromaWidth = ofGetWidth() / chroma.size();
//    for (int i = 0; i < chroma.size(); i++) {
//        float chromaMagnitude = chroma[i];
//        float x = i * chromaWidth;
//        ofDrawRectangle(x, ofGetHeight(), chromaWidth, -chromaMagnitude * 10.0);
//    }
}

//--------------------------------------------------------------
void ofApp::exit() {
    ofSoundStreamClose();
}

//--------------------------------------------------------------
void ofApp::audioIn(float* buffer, int bufferSize, int nChannels) {
    if (isRecording && recordedSamplesCount < recordedSamplesSize) {
        for (int i = 0; i < bufferSize; i++) {
            recordedSamples[recordedSamplesCount] = buffer[i];
            recordedSamplesCount++;
    //        float currentSample = buffer[i];
    //        phaseVocoder.addSample(currentSample);
    //        if (fft.process(currentSample)) {
    //            cout << "new fft?" << endl;
    //        };
        }
    }
}

//--------------------------------------------------------------
void ofApp::audioOut(float* buffer, int bufferSize, int nChannels) {
    for (int i = 0; i < bufferSize; i++) {
        float sample = 0;

        if (!isRecording && recordedSamplesCount > bufferSize) {
            sample = recordedSamples[recordedSamplesReadPoint];

            recordedSamplesReadPoint += 1;
            if (recordedSamplesReadPoint >= recordedSamplesCount) {
                recordedSamplesReadPoint = 0;
//                setPhaseVocoderMode();
//                phaseVocoder.setRandomPitchShift();
//                phaseVocoder.setRandomMode();
            }
        }
        
//        sample = pianoSamp.play();
//        float sample = osc.sinewave(frequency);
        phaseVocoder.addSample(sample);

        float currentSample = phaseVocoder.readSample();
        
//        dl.addSample(sample);
//        currentSample = dl.getSample();
        
//        float currentSample = myTapOut->getSample();
        
//        currentSample = delayLine.dl(sample, 8000, 0.3);
//        currentSample *= 3.0;
        
//        currentSample = currentSample * 10.0;
//        currentSample = ofClamp(currentSample, -1, 1);
        
        buffer[i * nChannels + 0] = currentSample;
        buffer[i * nChannels + 1] = currentSample;
    }
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key) {
    if (key == ' ' && !isRecording) {
        cout << "Begin recording" << endl;
        currState = AppState::RECORDING;
        recordedSamplesCount = 0;
        recordedSamplesReadPoint = 0;
        recordedSamples.clear();
        recordedSamples.resize(recordedSamplesSize);
        hasCleanedRecording = false;
        isRecording = true;
    }
    
    if (key >= 48 && key <= 57) {
        float digitVal = (float)(key - 48);
        feedbackAmount = digitVal / 10.0;
    }
    
    if (key == '1') {
        phaseVocoder.setPitchShift(powf(2.0, -12.0 / 12.0));
    }
    if (key == '2') {
        phaseVocoder.setPitchShift(powf(2.0, -6.0 / 12.0));
    }
    if (key == '3') {
        phaseVocoder.setPitchShift(powf(2.0, -3.0 / 12.0));
    }
    if (key == '4') {
        phaseVocoder.setPitchShift(powf(2.0, 0.0 / 12.0));
    }
    if (key == '5') {
        phaseVocoder.setPitchShift(powf(2.0, 3.0 / 12.0));
    }
    if (key == '6') {
        phaseVocoder.setPitchShift(powf(2.0, 6.0 / 12.0));
    }
    if (key == '7') {
        phaseVocoder.setPitchShift(powf(2.0, 9.0 / 12.0));
    }
    if (key == '8') {
        phaseVocoder.setPitchShift(powf(2.0, 12.0 / 12.0));
    }
    
    if (key == 'c') {
        shouldClearFeedbackImg = true;
    }
   
//    if (phaseVocoder.pitchShift > 0) {
//        phaseVocoder.setPitchShift(0);
//    } else {
//        phaseVocoder.setPitchShift(2);
//    }
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key) {
    if (key == ' ') {
        cout << "End recording" << endl;
        isRecording = false;
        cleanRecording();
        beginRunningPhases();
    }
    
    
    
    if (key == 'c') {
        shouldClearFeedbackImg = false;
    }
}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y) {
//    frequency = ofMap(x, 0, ofGetWidth(), 10, 22000);

//    float pitchShiftFactor = ofMap(x, 0, ofGetWidth(), -12, 12);
//    float pitchShift = powf(2.0, pitchShiftFactor / 12.0);
//    phaseVocoder.setPitchShift(pitchShift);
////
//    int pitchCount = (int) ofMap(x, 0, ofGetWidth(), 1, 20);
//    phaseVocoder.pitchCount = pitchCount;
    
//    float glitchAmount = ofMap(x, 0, ofGetWidth(), 0, 1);
//    setGlitchAmount(glitchAmount);
    
//    glitchAmount = ofMap(x, 0, ofGetWidth(), 0, 1);
//    glitchIntensity = mapSquared(y, 0, ofGetHeight(), 0, 1);
//    phaseVocoder.glitchAmount = glitchAmount;
//    phaseVocoder.glitchIntensity = glitchIntensity;
    
//    channelMix = ofMap(y, 0, ofGetHeight(), 1, 0);
    
//    int r = ofMap(x, 0, ofGetWidth(), 0, 255);
//    int b = ofMap(y, 0, ofGetHeight(), 0, 255);
//    ofSetColor(r, abs(r - b), b);
}

void ofApp::setGlitchAmount(float _glitchAmount) {
    float ellapsedTime = ofGetElapsedTimef() - phaseStartTime;
    
    glitchAmount = ofClamp(ofMap(ellapsedTime, 0, 45, 0, 1), 0, 1);
    glitchIntensity = ofClamp(mapSquared(ellapsedTime, 0, 45, 0, 2), 0, 2);
    
    phaseVocoder.glitchAmount = glitchAmount;
    phaseVocoder.glitchIntensity = glitchIntensity;
}

void ofApp::setPhaseVocoderMode() {
    if (glitchAmount < 0.25) {
        float pitchShiftDegree = floor(ofMap(glitchAmount, 0, 0.25, 0, 12));
        if (ofRandom(1.0) > 0.5) pitchShiftDegree*= -1;
        float pitchShfit = powf(2.0, (int)pitchShiftDegree / 12.0);
//
        phaseVocoder.setPitchShift(pitchShfit);
        phaseVocoder.setMode(simplePitchShift);
    } else if (glitchAmount < 0.5) {
//        phaseVocoder.setMode(multiPitchShift);
    }
}


void ofApp::beginRunningPhases() {
    currState = AppState::RUNNING_PHASE_ONE;
    phaseStartTime = ofGetElapsedTimef();
}

bool ofApp::isRunningPhases() {
    return currState == AppState::RUNNING_PHASE_ONE ||
           currState == AppState::RUNNING_PHASE_TWO ||
           currState == AppState::RUNNING_PHASE_THREE ||
           currState == AppState::RUNNING_PHASE_FOUR ||
           currState == AppState::RUNNING_PHASE_FIVE ||
           currState == AppState::RUNNING_PHASE_SIX;
}

void ofApp::randomSampleSkip() {
    int newReadPoint = floor(ofRandom(recordedSamplesCount));
    recordedSamplesReadPoint = newReadPoint;
}

// clean white space at beginning end of sample buffer
void ofApp::cleanRecording() {
    if (hasCleanedRecording) return;
    hasCleanedRecording = true;
    
    cout << "Begin cleaned recording" << endl;
    
    // amount to shave off start/end
    int cleanFromStart = (float)sampleRate / 20.0;
    int cleanFromEnd = (float)sampleRate / 20.0;
   
    // get avg sample value
    float sum = 0;
    for (int i = 0; i < recordedSamplesCount; i++) {
        sum += abs(recordedSamples[i]);
    }
    float avgSample = sum / (float)recordedSamplesCount;
    
    // use average sample value to guestimate how much more to shave off beginning/end
    int removeFromStart = cleanFromStart;
    for (int i = cleanFromStart; i < recordedSamplesCount && abs(recordedSamples[i]) < avgSample/2.0; i++) {
        removeFromStart++;
    }
    
    int removeFromEnd = cleanFromEnd;
    for (int i = recordedSamplesCount - 1 - cleanFromEnd; i >= 0 && abs(recordedSamples[i]) < avgSample/2.0; i--) {
        removeFromEnd++;
    }
    
    int cleanedSamplesLength = recordedSamplesCount - removeFromStart - removeFromEnd - cleanFromEnd;
    if (cleanedSamplesLength <= 0) return;
    
    // get new sample buffer with clean start/end
    vector<float> cleanedSamples(cleanedSamplesLength);
    for (int i = 0; i < cleanedSamplesLength; i++) {
        cleanedSamples[i] = recordedSamples[i + removeFromStart];
    }
    
    cout << "Avg sample: " << avgSample << endl;
    cout << "removeFromStart: " << removeFromStart << endl;
    cout << "removeFromEnd: " << removeFromEnd<< endl;
    cout << "Total samples: " << recordedSamplesCount << endl;
    cout << "Cleaned samples length: " << cleanedSamplesLength<< endl;

    // save the cleaned buffer
    recordedSamplesCount = cleanedSamplesLength;
    recordedSamples = cleanedSamples;
    cout << "Has cleaned recording" << endl;
}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button) {}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button) {}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button) {}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y) {}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y) {}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h) {}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg) {}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo) {}
