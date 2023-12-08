/*
* VideoAnalogiser - Command Line Utility for Analogising Digital Videos
* Maxim Hoxha 2023
* Colour system abstract class
* This software uses code of FFmpeg (http://ffmpeg.org) licensed under the LGPLv2.1 (http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html)
*/

#include "ColourSystem.h"

ColourSystem::ColourSystem()
{
	bcParams = &SystemI;
}

ColourSystem::ColourSystem(BroadcastSystems sys, bool interlace, double resonance, double prefilterMult, double phaseNoise, double scanlineJitter, double noiseExponent)
{
	switch (sys)
	{
	case BroadcastSystems::M:
		bcParams = &SystemM;
		break;
	case BroadcastSystems::N:
		bcParams = &SystemN;
		break;
	case BroadcastSystems::B:
		bcParams = &SystemB;
		break;
	case BroadcastSystems::G:
		bcParams = &SystemG;
		break;
	case BroadcastSystems::H:
		bcParams = &SystemH;
		break;
	default: //just because i live in the UK
	case BroadcastSystems::I:
		bcParams = &SystemI;
		break;
	case BroadcastSystems::D:
		bcParams = &SystemD;
		break;
	case BroadcastSystems::K:
		bcParams = &SystemK;
		break;
	case BroadcastSystems::L:
		bcParams = &SystemL;
		break;
	case BroadcastSystems::VHS525:
		bcParams = &TapeVHS525;
		break;
	case BroadcastSystems::VHS625:
		bcParams = &TapeVHS625;
		break;
	}
}

const char* GetColourSystemDescriptorString(ColourSystems cSys)
{
	switch (cSys)
	{
	default:
	case ColourSystems::PAL:
		return "PAL";
	case ColourSystems::NTSC:
		return "NTSC";
	case ColourSystems::SECAM:
		return "SECAM";
	}
}