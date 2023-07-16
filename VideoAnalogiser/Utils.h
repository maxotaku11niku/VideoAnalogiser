/*
* VideoAnalogiser - Command Line Utility for Analogising Digital Videos
* Maxim Hoxha 2023
* Mathematical utitlies, mostly relating to filters
* This software uses code of FFmpeg (http://ffmpeg.org) licensed under the LGPLv2.1 (http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html)
*/

#pragma once

typedef struct
{
	double* filter; //Note that this array is intended to be addressed with NEGATIVE numbers as well, because it makes handling them easier
	int len; //Length of components BEFORE and ON the zero point
	int backport; //Length of components AFTER the zero point. This is physically justifiable because one could just use delay lines on signals.
} FIRFilter;

typedef struct //Literally just made because I copied a bunch of code from my own AnalogueConvertEffect, which was written in C#. This structure made it easier to handle the signal arrays
{
	double* signal;
	int len;
} SignalPack;

#define CD_CLAMP(x, low, high)  (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))

FIRFilter MakeFIRFilter(double sampleRate, int size, double center, double width, double attenuation);
FIRFilter MakeNotch(FIRFilter filt);
SignalPack ApplyFIRFilter(SignalPack signal, FIRFilter fir);
SignalPack ApplyFIRFilterNotch(SignalPack signal, FIRFilter fir);
SignalPack ApplyFIRFilterCrosstalk(SignalPack signal, FIRFilter fir, double crosstalk);
SignalPack ApplyFIRFilterShift(SignalPack signal, FIRFilter fir, double sampleTime, double centerangfreq);
SignalPack ApplyFIRFilterNotchCrosstalk(SignalPack signal, FIRFilter fir, double crosstalk);
SignalPack ApplyFIRFilterCrosstalkShift(SignalPack signal, FIRFilter fir, double crosstalk, double sampleTime, double centerangfreq);
SignalPack ApplyFIRFilterNotchShift(SignalPack signal, FIRFilter fir, double sampleTime, double centerangfreq);
SignalPack ApplyFIRFilterNotchCrosstalkShift(SignalPack signal, FIRFilter fir, double crosstalk, double sampleTime, double centerangfreq);