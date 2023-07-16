/*
* VideoAnalogiser - Command Line Utility for Analogising Digital Videos
* Maxim Hoxha 2023
* SECAM encoder/decoder
* This software uses code of FFmpeg (http://ffmpeg.org) licensed under the LGPLv2.1 (http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html)
*/

#pragma once
#include <random>
#include "ColourSystem.h"

#define SUBCARRIER_START_TIME 0.4e-6

const double RGBtoYDbDrConversionMatrix[9] = { 0.299,  0.587, 0.114,
                                              -0.45,  -0.883, 1.333,
                                              -1.333,  1.116, 0.217 };

const double YDbDrtoRGBConversionMatrix[9] = { 1.0,  0.0,               -0.525912630661865,
                                               1.0, -0.129132898809509,  0.267899328207599,
                                               1.0,  0.664679059978955,  0.0 };

class SECAMSystem : public ColourSystem
{
public:
    SECAMSystem(BroadcastSystems sys, bool interlace, double resonance, double prefilterMult);

    virtual SignalPack Encode(FrameData imgdat, int interlaceField) override;
    virtual FrameData Decode(SignalPack signal, int interlaceField, double crosstalk, double phaseError, double phaseNoise, double scanlineJitter) override;
private:
    std::mt19937_64 rng;
    bool interlaced;
    int fieldScanlines;
    int activeWidth;
    int* boundaryPoints;
    int* activeSignalStarts;
    double sampleRate;
    double sampleTime;
    FIRFilter mainfir;
    FIRFilter dbfir;
    FIRFilter drfir;
    FIRFilter colfir;
    FIRFilter lumaprefir;
    FIRFilter chromaprefir;
    double* DbSignalI;
    double* DbSignalQ;
    double* DrSignalI;
    double* DrSignalQ;
};