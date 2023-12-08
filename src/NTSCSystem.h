/*
* VideoAnalogiser - Command Line Utility for Analogising Digital Videos
* Maxim Hoxha 2023
* NTSC encoder/decoder
* This software uses code of FFmpeg (http://ffmpeg.org) licensed under the LGPLv2.1 (http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html)
*/

#define _USE_MATH_DEFINES
#pragma once
#include <random>
#include <math.h>
#include "ColourSystem.h"
#include "MultiOctaveNoiseGen.h"

const double RGBtoYIQConversionMatrix[9] = { 0.299,    0.587,  0.114,
                                             0.5959, -0.2746, -0.3213,
                                             0.2115, -0.5227,  0.3112 };

const double YIQtoRGBConversionMatrix[9] = { 1.0,  0.956,  0.619,
                                             1.0, -0.272, -0.647,
                                             1.0, -1.106,  1.703 };

constexpr double chromaPhase = 33.0 * (M_PI / 180.0);

class NTSCSystem : public ColourSystem
{
public:
    NTSCSystem(BroadcastSystems sys, bool interlace, double resonance, double prefilterMult, double phaseNoise, double scanlineJitter, double noiseExponent);

    virtual SignalPack Encode(FrameData imgdat, int interlaceField) override;
    virtual FrameData Decode(SignalPack signal, int interlaceField, double crosstalk) override;
private:
    MultiOctaveNoiseGen* jitGen;
    MultiOctaveNoiseGen* phNoiseGen;
    bool interlaced;
    int fieldScanlines;
    int activeWidth;
    int* boundaryPoints;
    int* activeSignalStarts;
    double sampleRate;
    double sampleTime;
    FIRFilter mainfir;
    FIRFilter qfir;
    FIRFilter ifir;
    FIRFilter lumaprefir;
    FIRFilter qprefir;
    FIRFilter iprefir;
};