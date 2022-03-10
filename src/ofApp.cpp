#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup() {
    bufferSize = 512;
    sampleRate = 44100;
    
    frequency = 440;
    
    ofxMaxiSettings::setup(sampleRate, 2, bufferSize);
    
    fftSize = 1024;
    fft.setup(fftSize, bufferSize, 256);

    ofSoundStreamSetup(2, 0, this, sampleRate, bufferSize, 4);
}

//--------------------------------------------------------------
void ofApp::update() {}

//--------------------------------------------------------------
void ofApp::draw() {
    int fftBins = fftSize / 2;
    float bandWidth = ofGetWidth()/ (float)fftBins;
    float* magnitudes = fft.magnitudes;
    
    for (int i = 0; i < fftBins; i++) {
        float fftMagnitude = magnitudes[i];
        float x = i * bandWidth;
        ofDrawRectangle(x, ofGetHeight() / 2, bandWidth, -fftMagnitude);
    }
}

//--------------------------------------------------------------
void ofApp::exit() {
    ofSoundStreamClose();
}

//--------------------------------------------------------------
void ofApp::audioIn(float* buffer, int bufferSize, int nChannels) {}

//--------------------------------------------------------------
void ofApp::audioOut(float* buffer, int bufferSize, int nChannels) {
    for (int i = 0; i < bufferSize; i++) {
        float currentSample = osc.sinewave(frequency);
        
        fft.process(currentSample);

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
    frequency = ofMap(x, 0, ofGetWidth(), 20, 4000);
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
