/*
* VideoAnalogiser - Command Line Utility for Analogising Digital Videos
* Maxim Hoxha 2023
* Multiple octave noise generator
* This software uses code of FFmpeg (http://ffmpeg.org) licensed under the LGPLv2.1 (http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html)
*/

#pragma once
#include <random>

class MultiOctaveNoiseGen
{
public:
	MultiOctaveNoiseGen(int numOct, double distCenter, double distWidth, double exponent);
	double GenNoise();
private:
	std::mt19937_64 rng;
	std::uniform_real_distribution<> dist;
	int numOctaves;
	double noiseAmplitudes[32];
	double noiseFilters[32];
	double noiseChannels[32]; //fixed size for efficiency
};