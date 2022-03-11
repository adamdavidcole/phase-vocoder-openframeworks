#pragma once

#include "ofMain.h"
#include "ofxMaxim.h"
#include "ofxGui.h"
#include "CircularBuffer.hpp"
#include "PhaseVocoder.hpp"

class ofApp : public ofBaseApp {
   public:
    void setup();
    void update();
    void draw();
    void exit();
    
    void audioIn(float* buffer, int bufferSize, int nChannels);
    void audioOut(float* buffer, int bufferSize, int nChannels);

    void keyPressed(int key);
    void keyReleased(int key);
    void mouseMoved(int x, int y);
    void mouseDragged(int x, int y, int button);
    void mousePressed(int x, int y, int button);
    void mouseReleased(int x, int y, int button);
    void mouseEntered(int x, int y);
    void mouseExited(int x, int y);
    void windowResized(int w, int h);
    void dragEvent(ofDragInfo dragInfo);
    void gotMessage(ofMessage msg);
    
    int bufferSize;
    int sampleRate;
    
    int windowSize;
    int hopSize;

    float frequency;
    int fftSize;
    int fftBins;
    float binFreq;
    
    maxiOsc osc;
    maxiSample pianoSamp;
//
//    vector<float> constQ;
//    vector<float> chroma;

    PhaseVocoder phaseVocoder;
};

