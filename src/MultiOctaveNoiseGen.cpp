/*
* VideoAnalogiser - Command Line Utility for Analogising Digital Videos
* Maxim Hoxha 2023
* Multiple octave noise generator
* This software uses code of FFmpeg (http://ffmpeg.org) licensed under the LGPLv2.1 (http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html)
*/

#include <math.h>
#include "MultiOctaveNoiseGen.h"

//noise amplitudes go as width^exponent
MultiOctaveNoiseGen::MultiOctaveNoiseGen(int numOct, double distCenter, double distWidth, double exponent)
{
	dist = std::uniform_real_distribution<>(distCenter - distWidth, distCenter + distWidth);
	numOctaves = numOct;
	exponent -= 1.0;
	double ampCorr;
	if (exponent == 0.0)
	{
		ampCorr = 1.0 / ((double)numOct);
	}
	else
	{
		ampCorr = ((1.0 - exp2(exponent)) / (1.0 - exp2(exponent * numOct)));
	}
	for (int i = 0; i < numOct; i++)
	{
		noiseFilters[i] = 1.0 - exp2((double)-i);
		noiseAmplitudes[i] = exp2(exponent * i) * ampCorr;
		noiseChannels[i] = 0.0;
	}
	for (int i = 0; i < 69420; i++) //this number is totally random :)
	{
		GenNoise(); //burn-in
	}
}

double MultiOctaveNoiseGen::GenNoise()
{
	double outnum = 0.0;
	for (int i = 0; i < numOctaves; i++)
	{
		const double noisecomp = dist(rng) + noiseChannels[i] * noiseFilters[i];
		outnum += noisecomp * noiseAmplitudes[i];
	    noiseChannels[i] = noisecomp;
	}
	return outnum;
}