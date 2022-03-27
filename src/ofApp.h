#pragma once

#include "ofMain.h"
#include "ofxMaxim.h"
#include "ofxGui.h"
#include "ofxShader.h"
#include "ofxShaderFilter.h"
#include "ofxVideoRecorder.h"

#include "CircularBuffer.hpp"
#include "PhaseVocoder.hpp"
#include "DelayLine.hpp"

enum class AppState {
    IDLE, RUNNING_PHASE_ONE, RUNNING_PHASE_TWO, RUNNING_PHASE_THREE, RUNNING_PHASE_FOUR, RUNNING_PHASE_FIVE, RUNNING_PHASE_SIX, RECORDING
};

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
    
    void drawInstructions();
    
    
    AppState currState;
    float phaseStartTime;
    void beginRunningPhases();
    bool isRunningPhases();
    void randomSampleSkip();
    
    int bufferSize;
    int sampleRate;
    int channels;
    
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
    float phaseVocoderVolume;
    
    int recordedSamplesSize;
    int recordedSamplesCount;
    int recordedSamplesReadPoint;
    vector<float> recordedSamples;
    bool isRecording;
    
    float channelMix;
    
    maxiDelayline delayLine;
    
    int camWidth;
    int camHeight;
    
    ofVideoGrabber webcam;
    ofPixels pixelsToDraw;
    ofTexture myTexture;
    ofImage feedbackImg;
    bool shouldClearFeedbackImg;
    ofPixels feedbackPixels;

    
    ofPlanePrimitive plane;
    int planeWidth;
    int planeHeight;
    int planeX;
    int planeY;
    
    ofxShader shader;
    
    float glitchIntensity;
    float glitchAmount;
    float feedbackAmount;
    bool hasMaxedFeedbackBeforeCalmDown = false;
    
    void cleanRecording();
    bool hasCleanedRecording;
//    DelayLine dl;
    
    // SCREEN RECORDING
    ofVideoGrabber      vidGrabber;
    ofxVideoRecorder    vidRecorder;
    ofSoundStream       soundStream;
    
    bool bRecording;
    string fileName;
    string fileExt;

    void recordingComplete(ofxVideoRecorderOutputFileCompleteEventArgs& args);

    ofFbo recordFbo;
    ofPixels recordPixels;
};

