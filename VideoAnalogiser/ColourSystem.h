/*
* VideoAnalogiser - Command Line Utility for Analogising Digital Videos
* Maxim Hoxha 2023
* Colour system abstract class
* This software uses code of FFmpeg (http://ffmpeg.org) licensed under the LGPLv2.1 (http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html)
*/

#pragma once
#include <math.h>
#include "BroadcastStandard.h"
#include "Utils.h"

#define PREFILTER_RESONANCE 2.0

typedef struct
{
	int* image; //assumed to be an array of 0xAARRGGBB
	int width;
	int height;
} FrameData;

enum ColourSystems
{
	PAL,
	NTSC,
	SECAM
};

const char* GetColourSystemDescriptorString(ColourSystems cSys);

class ColourSystem
{
public:
	ColourSystem();
	ColourSystem(BroadcastSystems sys, bool interlace, double resonance, double prefilterMult, double phaseNoise, double scanlineJitter, double noiseExponent);

	const BroadcastStandard* bcParams;

	virtual SignalPack Encode(FrameData imgdat, int interlaceField) = 0;
	virtual FrameData Decode(SignalPack signal, int interlaceField, double crosstalk) = 0;

protected:
	const double* RGBtoYCCConversionMatrix;
	const double* YCCtoRGBConversionMatrix;

	//These two functions help translate logical RGB values into real values, but it assumes that the video is encoded for the sRGB colourspace. Most videos will fit this description as most people wouldn't really care about that stuff.
	inline double SRGBGammaTransform(double val)
	{
		return val > 0.04045 ? pow((val + 0.055) / 1.055, 2.4) : (val / 12.92);
	}

	inline double SRGBInverseGammaTransform(double val)
	{
		return val > 0.0031308 ? (pow(val, 1 / 2.4) * 1.055) - 0.055 : val * 12.92;
	}
};