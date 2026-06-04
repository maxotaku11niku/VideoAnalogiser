/*
* VideoAnalogiser - Command Line Utility for Analogising Digital Videos
* Maxim Hoxha 2023-2026
* PAL encoder/decoder
* This software uses code of FFmpeg (http://ffmpeg.org) licensed under the LGPLv2.1 (http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html)
*/

#pragma once
#include <random>
#include "ColourSystem.h"
#include "MultiOctaveNoiseGen.h"

const double RGBtoYUVConversionMatrix[9] = { 0.299,    0.587,    0.114,
                                            -0.14713, -0.28886,  0.436,
                                             0.615,   -0.51499, -0.10001 };

const double YUVtoRGBConversionMatrix[9] = { 1.0,  0.0,      1.13983,
                                             1.0, -0.39465, -0.58060,
                                             1.0,  2.03211,  0.0 };

class PALSystem : public ColourSystem
{
public:
	PALSystem(BroadcastSystems sys, bool interlace, double resonance, double prefilterMult, double phaseNoise, double scanlineJitter, double noiseExponent);

    virtual SignalPack Encode(FrameData imgdat, int field) override;
    virtual FrameData Decode(SignalPack signal, int field, double crosstalk) override;
    virtual SignalPack AddText(SignalPack signal, const char* text, double x, int y, bool yRelativeToBottom) override;
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
    FIRFilter colfir;
    FIRFilter lumaprefir;
    FIRFilter chromaprefir;
    float* USignal;
    float* VSignal;
    float* USignalPreAlt;
    float* VSignalPreAlt;
};
