/*
* VideoAnalogiser - Command Line Utility for Analogising Digital Videos
* Maxim Hoxha 2023
* Broadcast standard definitions
* This software uses code of FFmpeg (http://ffmpeg.org) licensed under the LGPLv2.1 (http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html)
*/

#pragma once

extern "C"
{
#include <libavutil/rational.h>
}

typedef struct
{
	double mainBandwidth;
	double sideBandwidth;
	double chromaBandwidthLower;
	double chromaBandwidthUpper;
	double chromaCarrierFrequency;
	int scanlines;
	int videoScanlines;
	double framerate;
	AVRational ratFrametime; //For FFmpeg code
	double activeTime;
	double carrierAngFreq;
	double frameTime;
	double scanlineTime;
	//SECAM only
	double chromaCarrierFrequencyDb;
	double chromaCarrierFrequencyDr;
	double chromaBandwidthLowerDb;
	double chromaBandwidthUpperDb;
	double chromaBandwidthLowerDr;
	double chromaBandwidthUpperDr;
	double deltaAngFreqDb;
	double deltaAngFreqDr;
	double carrierAngFreqDb;
	double carrierAngFreqDr;
} BroadcastStandard;


enum BroadcastSystems
{
	M,
	N,
	B,
	G,
	H,
	I,
	D,
	K,
	L,
	VHS525,
	VHS625
};

extern const BroadcastStandard SystemM;
extern const BroadcastStandard SystemN;
extern const BroadcastStandard SystemB;
extern const BroadcastStandard SystemG;
extern const BroadcastStandard SystemH;
extern const BroadcastStandard SystemI;
extern const BroadcastStandard SystemD;
extern const BroadcastStandard SystemK;
extern const BroadcastStandard SystemL;
extern const BroadcastStandard TapeVHS525;
extern const BroadcastStandard TapeVHS625;

const char* GetBroadcastSystemDescriptorString(BroadcastSystems bSys);
void DisplayBroadcastSystemInformation(BroadcastSystems bSys);
