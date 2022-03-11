#include "ofApp.h"

float mtofArray[] = {0, 8.661957, 9.177024, 9.722718, 10.3, 10.913383, 11.562325, 12.25, 12.978271, 13.75, 14.567617, 15.433853, 16.351599, 17.323914, 18.354048, 19.445436, 20.601723, 21.826765, 23.124651, 24.5, 25.956543, 27.5, 29.135235, 30.867706, 32.703197, 34.647827, 36.708096, 38.890873, 41.203445, 43.65353, 46.249302, 49., 51.913086, 55., 58.27047, 61.735413, 65.406395, 69.295654, 73.416191, 77.781746, 82.406891, 87.30706, 92.498604, 97.998856, 103.826172, 110., 116.540939, 123.470825, 130.81279, 138.591309, 146.832382, 155.563492, 164.813782, 174.61412, 184.997208, 195.997711, 207.652344, 220., 233.081879, 246.94165, 261.62558, 277.182617,293.664764, 311.126984, 329.627563, 349.228241, 369.994415, 391.995422, 415.304688, 440., 466.163757, 493.883301, 523.25116, 554.365234, 587.329529, 622.253967, 659.255127, 698.456482, 739.988831, 783.990845, 830.609375, 880., 932.327515, 987.766602, 1046.502319, 1108.730469, 1174.659058, 1244.507935, 1318.510254, 1396.912964, 1479.977661, 1567.981689, 1661.21875, 1760., 1864.655029, 1975.533203, 2093.004639, 2217.460938, 2349.318115, 2489.015869, 2637.020508, 2793.825928, 2959.955322, 3135.963379, 3322.4375, 3520., 3729.31, 3951.066406, 4186.009277, 4434.921875, 4698.63623, 4978.031738, 5274.041016, 5587.651855, 5919.910645, 6271.926758, 6644.875, 7040., 7458.620117, 7902.132812, 8372.018555, 8869.84375, 9397.272461, 9956.063477, 10548.082031, 11175.303711, 11839.821289, 12543.853516, 13289.75};

void printFloatVector(vector<float> vec) {
    std::cout << "[ ";
    for (float i : vec) {
        std::cout << i << ' ';
    }
    std::cout << "]" << endl;
}

//--------------------------------------------------------------
void ofApp::setup() {
    pianoSamp.load(ofToDataPath("caine_03.wav"));
    
    bufferSize = 512;
    sampleRate = 44100;
    
    frequency = 440;
    
    ofxMaxiSettings::setup(sampleRate, 2, bufferSize);
    
    fftSize = 1024;
    fftBins = fftSize / 2;
    binFreq = sampleRate / fftSize;
    
    windowSize = bufferSize / 2;
    hopSize = windowSize / 2;
    
    phaseVocoder.setup(fftSize, windowSize, hopSize);
//
//    constQ.resize(128);
//    chroma.resize(12);

    ofSoundStreamSetup(2, 1, this, sampleRate, bufferSize, 4);
    
    CircularBuffer testBuffer(8);
    
    testBuffer.push(0);
    testBuffer.push(1);
    testBuffer.push(2);
    testBuffer.push(3);
    testBuffer.push(4);
    testBuffer.push(5);
    testBuffer.push(6);
    testBuffer.push(7);
    testBuffer.push(8);
    testBuffer.push(9);
    testBuffer.push(10);
        
    vector<float> window = testBuffer.getWindow(6);
    printFloatVector(window);
}

//--------------------------------------------------------------
void ofApp::update() {

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
}

//--------------------------------------------------------------
void ofApp::draw() {
    float bandWidth = ofGetWidth()/ (float)fftBins;
    float* magnitudes = phaseVocoder.fft.magnitudes;
    
    for (int i = 0; i < fftBins; i++) {
        float fftMagnitude = magnitudes[i];
        float x = i * bandWidth;
        ofDrawRectangle(x, ofGetHeight() / 2, bandWidth, -fftMagnitude);
    }
    
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
    for (int i = 0; i < bufferSize; i++) {
//        float currentSample = buffer[i];
//        if (fft.process(currentSample)) {
//            cout << "new fft?" << endl;
//        };
    }
}

//--------------------------------------------------------------
void ofApp::audioOut(float* buffer, int bufferSize, int nChannels) {
    for (int i = 0; i < bufferSize; i++) {
//        float sample = pianoSamp.play();
        float sample = osc.sinewave(frequency);
        phaseVocoder.addSample(sample);

        float currentSample = phaseVocoder.readSample();
        
        currentSample *= 0.05;

        buffer[i * nChannels + 0] = currentSample;
        buffer[i * nChannels + 1] = currentSample;
    }
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key) {}

//--------------------------------------------------------------
void ofApp::keyReleased(int key) {}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y) {
    frequency = ofMap(x, 0, ofGetWidth(), 10, 22000);
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
