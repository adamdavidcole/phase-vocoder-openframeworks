//
//  smpPitchShift.hpp
//  sing-to-me
//
//  Created by Adam Cole on 3/11/22.
//

#ifndef smpPitchShift_hpp
#define smpPitchShift_hpp

#include <stdio.h>

void smbPitchShift(float pitchShift, long numSampsToProcess, long fftFrameSize, long osamp, float sampleRate, float *indata, float *outdata);
void smbFft(float *fftBuffer, long fftFrameSize, long sign);
double smbAtan2(double x, double y);

#endif /* smpPitchShift_hpp */
