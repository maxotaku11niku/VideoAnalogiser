/*
* VideoAnalogiser - Command Line Utility for Analogising Digital Videos
* Maxim Hoxha 2023-2026
* PAL encoder/decoder
* This software uses code of FFmpeg (http://ffmpeg.org) licensed under the LGPLv2.1 (http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html)
*/

#include <iostream>
#include "PALSystem.h"
#include "VHSFont.h"

PALSystem::PALSystem(BroadcastSystems sys, bool interlace, double resonance, double prefilterMult, double phaseNoise, double scanlineJitter, double noiseExponent)
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

	RGBtoYCCConversionMatrix = RGBtoYUVConversionMatrix;
	YCCtoRGBConversionMatrix = YUVtoRGBConversionMatrix;

	interlaced = interlace;
	activeWidth = FIXEDWIDTH;
	fieldScanlines = interlace ? bcParams->videoScanlines / 2 : bcParams->videoScanlines;
	boundaryPoints = new int[fieldScanlines + 1]; //Boundaries of the scanline signals
	activeSignalStarts = new int[fieldScanlines]; //Start points of the active parts
	sampleRate = activeWidth / bcParams->activeTime; //Correction for the fact that the signal we've created only has active scanlines.
	sampleTime = bcParams->activeTime / (double)activeWidth;
	double realScanlineTime = 1.0 / (double)(fieldScanlines * bcParams->framerate);
	int signalLen = (int)(activeWidth * fieldScanlines * (realScanlineTime / bcParams->activeTime)); //To get a good analogue feel, we must limit the vertical resolution; the horizontal resolution will be limited as we decode the distorted signal.

	std::cout << "Creating decode filters..." << std::endl;

	mainfir = MakeFIRFilter(sampleRate, 256, (bcParams->mainBandwidth - bcParams->sideBandwidth) / 2.0, bcParams->mainBandwidth + bcParams->sideBandwidth, resonance);
	colfir = MakeFIRFilter(sampleRate, 256, (bcParams->chromaBandwidthUpper - bcParams->chromaBandwidthLower) / 2.0, bcParams->chromaBandwidthLower + bcParams->chromaBandwidthUpper, resonance);

	std::cout << "Creating prefilters..." << std::endl;

	lumaprefir = MakeFIRFilter(sampleRate, 256, 0.0, 2.0 * bcParams->mainBandwidth * prefilterMult, PREFILTER_RESONANCE);
	chromaprefir = MakeFIRFilter(sampleRate, 256, 0.0, 2.0 * bcParams->chromaBandwidthLower * prefilterMult, PREFILTER_RESONANCE);

	USignal = new float[signalLen];
	VSignal = new float[signalLen];
	USignalPreAlt = new float[signalLen];
	VSignalPreAlt = new float[signalLen];

	jitGen = new MultiOctaveNoiseGen(11, 0.0, scanlineJitter * activeWidth, noiseExponent);
	phNoiseGen = new MultiOctaveNoiseGen(11, 0.0, phaseNoise, noiseExponent);
}

SignalPack PALSystem::Encode(FrameData imgdat, int field)
{
	double realActiveTime = bcParams->activeTime;
	double realScanlineTime = bcParams->scanlineTime;
	int signalLen = (int)(imgdat.width * fieldScanlines * (realScanlineTime/realActiveTime)); //To get a good analogue feel, we must limit the vertical resolution; the horizontal resolution will be limited as we decode the distorted signal.
	float* signalOut = new float[signalLen];
	double R = 0.0;
	double G = 0.0;
	double B = 0.0;
	double Y = 0.0;
	double U = 0.0;
	double V = 0.0;
	int pos = 0;
	double phaseAlternate = 1.0; //Why this is called PAL in the first place
	int remainingSync = 0;
	double sampleTime = realActiveTime / (double)imgdat.width;

	boundaryPoints[0] = 0; //Beginning of the signal
	boundaryPoints[fieldScanlines] = signalLen; //End of the signal
	for (int i = 1; i < fieldScanlines; i++) //Rest of the signal
	{
		boundaryPoints[i] = (i * signalLen) / fieldScanlines;
	}

	for (int i = 0; i < fieldScanlines; i++) //Where the active signal starts
	{
		activeSignalStarts[i] = (int)((((double)i * (double)signalLen) / (double)fieldScanlines) + ((realScanlineTime - realActiveTime) / (2 * realActiveTime)) * imgdat.width) - boundaryPoints[i];
	}

	int* imgColours = imgdat.image;
	int currentScanline;
	double finSamp = 0.0;
	int w = imgdat.width;
	int col;
	int interlaceField = field & 1;
	double carrierAngFreq = bcParams->carrierAngFreq;
	SignalPack Ysig = { new float[signalLen], signalLen };
	SignalPack Usig = { new float[signalLen], signalLen };
	SignalPack Vsig = { new float[signalLen], signalLen };
	//Make component signals
	for (int i = 0; i < fieldScanlines; i++) //Only generate active scanlines
	{
		currentScanline = interlaced ? (i * 2 + interlaceField) % bcParams->videoScanlines : i;
		for (int j = 0; j < activeSignalStarts[i]; j++) //Front porch, ignore sync signal because we don't see its results
		{
			Ysig.signal[pos] = 0.0f;
			Usig.signal[pos] = 0.0f;
			Vsig.signal[pos] = 0.0f;
			pos++;
		}
		for (int j = 0; j < w; j++) //Active signal
		{
			col = imgColours[currentScanline * w + j];
			R = ((col & 0x00FF0000) >> 16) / 255.0;
			G = ((col & 0x0000FF00) >> 8) / 255.0;
			B = (col & 0x000000FF) / 255.0;
			R = SRGBGammaTransform(R); //SRGB correction
			G = SRGBGammaTransform(G);
			B = SRGBGammaTransform(B);
			R = pow(R, 1.0/2.8); //Gamma correction
			G = pow(G, 1.0/2.8);
			B = pow(B, 1.0/2.8);
			Ysig.signal[pos] = RGBtoYUVConversionMatrix[0] * R + RGBtoYUVConversionMatrix[1] * G + RGBtoYUVConversionMatrix[2] * B; //Encode Y, U and V
			Usig.signal[pos] = RGBtoYUVConversionMatrix[3] * R + RGBtoYUVConversionMatrix[4] * G + RGBtoYUVConversionMatrix[5] * B;
			Vsig.signal[pos] = RGBtoYUVConversionMatrix[6] * R + RGBtoYUVConversionMatrix[7] * G + RGBtoYUVConversionMatrix[8] * B;
			pos++;
		}
		while (pos < boundaryPoints[i + 1]) //Back porch, ignore sync signal because we don't see its results
		{
			Ysig.signal[pos] = 0.0f;
			Usig.signal[pos] = 0.0f;
			Vsig.signal[pos] = 0.0f;
			pos++;
		}
	}

	//Prefilter signals
	SignalPack filtYsig = ApplyFIRFilter(Ysig, lumaprefir);
	SignalPack filtUsig = ApplyFIRFilter(Usig, chromaprefir);
	SignalPack filtVsig = ApplyFIRFilter(Vsig, chromaprefir);
	pos = 0;
	double frameAlternation = field & 2 ? -1.0 : 1.0;
	double fieldPhaseAdv = fmod((field % 2500) * carrierAngFreq * bcParams->frameTime, 2.0 * M_PI);
	//Composite component signals
	for (int i = 0; i < fieldScanlines; i++)
	{
		if ((i % 2) == 1) //Do phase alternation
		{
			phaseAlternate = -frameAlternation;
		}
		else phaseAlternate = frameAlternation;
		double time = 0.0;
		while (pos < boundaryPoints[i + 1])
		{
			double time = pos * sampleTime;
			signalOut[pos] = filtYsig.signal[pos] + filtUsig.signal[pos] * sin(carrierAngFreq * time + fieldPhaseAdv) + phaseAlternate * filtVsig.signal[pos] * cos(carrierAngFreq * time + fieldPhaseAdv); //Add chroma via QAM
			pos++;
		}
	}

	delete[] Ysig.signal;
	delete[] Usig.signal;
	delete[] Vsig.signal;
	delete[] filtYsig.signal;
	delete[] filtUsig.signal;
	delete[] filtVsig.signal;

    return { signalOut, signalLen };
}

FrameData PALSystem::Decode(SignalPack signal, int field, double crosstalk)
{
    double realActiveTime = bcParams->activeTime;
    double realScanlineTime = bcParams->scanlineTime;
    double realFrameTime = (interlaced ? 2.0 : 1.0) / bcParams->framerate;
    int R = 0;
    int G = 0;
    int B = 0;
    double Y = 0.0;
    double U = 0.0;
    double V = 0.0;
    int pos = 0;
    int posdel = 0;
    double sigNum = 0.0;
    double blendStr = 1.0 - crosstalk;
    double sampleTime = realActiveTime / (double)activeWidth;
    
    SignalPack colsignal = ApplyFIRFilterCrosstalkShift(signal, colfir, crosstalk, sampleTime, bcParams->carrierAngFreq);
    SignalPack newSignal = ApplyFIRFilter(signal, mainfir);

	//Extract QAM colour signals
    double carrierAngFreq = bcParams->carrierAngFreq;
	double fieldPhaseAdv = fmod((field % 2500) * carrierAngFreq * bcParams->frameTime, 2.0 * M_PI);
	double frameAlternation = field & 2 ? -1.0 : 1.0;
	for (int i = 0; i < fieldScanlines; i++)
	{
		double phOffs = phNoiseGen->GenNoise();
		double phaseAdv = fmod(phOffs + fieldPhaseAdv, 2.0 * M_PI);
		while (pos < boundaryPoints[i + 1])
		{
			double time = pos * sampleTime;
			USignalPreAlt[pos] = colsignal.signal[pos] * sin(carrierAngFreq * time + phaseAdv) * 2.0;
			VSignalPreAlt[pos] = frameAlternation * colsignal.signal[pos] * cos(carrierAngFreq * time + phaseAdv) * 2.0;
			pos++;
		}
	}

    SignalPack finalSignal = ApplyFIRFilterNotchCrosstalkShift(newSignal, colfir, crosstalk, sampleTime, carrierAngFreq);
    SignalPack finalUSignal = ApplyFIRFilter({ USignalPreAlt, signal.len }, colfir);
    SignalPack finalVSignal = ApplyFIRFilter({ VSignalPreAlt, signal.len }, colfir);

    FrameData writeToSurface = { new int[activeWidth * fieldScanlines], activeWidth, fieldScanlines };

    for (int i = 0; i < fieldScanlines; i++) //Where the active signal starts
    {
        activeSignalStarts[i] = (int)((((double)i * (double)signal.len) / (double)fieldScanlines) + ((realScanlineTime - realActiveTime) / (2 * realActiveTime)) * activeWidth);
    }

	//Account for phase-alternation
    double alt = 0.0;
    pos = activeSignalStarts[0];
    for (int j = 0; j < writeToSurface.width; j++) //We assume the chroma signal in all blanking periods is zero
    {
        USignal[pos] = finalUSignal.signal[pos] / 2.0;
        VSignal[pos] = finalVSignal.signal[pos] / 2.0;
        pos++;
        posdel++;
    }
    for (int i = 1; i < fieldScanlines; i++) //Simulate a delay line
    {
        pos = activeSignalStarts[i];
        posdel = activeSignalStarts[i - 1];
        alt = (i % 2) == 0 ? -1.0 : 1.0;
        for (int j = 0; j < writeToSurface.width; j++)
        {
            USignal[pos] = (finalUSignal.signal[posdel] + finalUSignal.signal[pos]) / 2.0;
            VSignal[pos] = alt * (finalVSignal.signal[posdel] - finalVSignal.signal[pos]) / 2.0;
            pos++;
            posdel++;
        }
    }

    int* surfaceColours = writeToSurface.image;
    int curjit = 0;
	int finCol = 0;
	double dR = 0.0;
	double dG = 0.0;
	double dB = 0.0;
	//Write decoded signals to our frame
    for (int i = 0; i < fieldScanlines; i++)
    {
		curjit = (int)jitGen->GenNoise();
		if (curjit > 100) curjit = 100;
		if (curjit < -100) curjit = -100; //Limit jitter distance to prevent buffer overflow
        pos = activeSignalStarts[i] + curjit;
        for (int j = 0; j < writeToSurface.width; j++) //Decode active signal region only
        {
            Y = finalSignal.signal[pos];
            U = USignal[pos];
            V = VSignal[pos];
			dR = pow(YUVtoRGBConversionMatrix[0] * Y + YUVtoRGBConversionMatrix[2] * V, 2.8);
			dG = pow(YUVtoRGBConversionMatrix[3] * Y + YUVtoRGBConversionMatrix[4] * U + YUVtoRGBConversionMatrix[5] * V, 2.8);
			dB = pow(YUVtoRGBConversionMatrix[6] * Y + YUVtoRGBConversionMatrix[7] * U, 2.8);
            R = (int)(CD_CLAMP(SRGBInverseGammaTransform(dR), 0.0, 1.0) * 255.0);
            G = (int)(CD_CLAMP(SRGBInverseGammaTransform(dG), 0.0, 1.0) * 255.0);
            B = (int)(CD_CLAMP(SRGBInverseGammaTransform(dB), 0.0, 1.0) * 255.0);
			finCol = 0xFF000000;
			finCol |= R << 16;
			finCol |= G << 8;
			finCol |= B;
            surfaceColours[i * writeToSurface.width + j] = finCol;
            pos++;
        }
    }

	delete[] colsignal.signal;
	delete[] newSignal.signal;
	delete[] finalSignal.signal;
	delete[] finalUSignal.signal;
	delete[] finalVSignal.signal;

    return writeToSurface;
}

SignalPack PALSystem::AddText(SignalPack signal, const char* text, double x, int y, bool yRelativeToBottom)
{
	unsigned char curCh = *text++;
	int chCount = 0;
	int startY = y;
	if (yRelativeToBottom) startY = fieldScanlines - y;
	int finalScanline = startY + 14;
	if (finalScanline > fieldScanlines) finalScanline = fieldScanlines;
	int actualStartY = startY;
	if (actualStartY < 0) actualStartY = 0;
	double realActiveTime = bcParams->activeTime;
	double realScanlineTime = 1.0 / (double)(fieldScanlines * bcParams->framerate);
	double scanlineLength = FIXEDWIDTH * (realScanlineTime/realActiveTime);
	double scanlineIncPerSample = 1.0 / (scanlineLength * VHS_FONT_GLYPH_WIDTH);
	float* sig = signal.signal;
	while (curCh != 0)
	{
		if (curCh < 0x80)
		{
			const unsigned short* glyph = asciiPtrs[curCh];
			for (int i = actualStartY; i < finalScanline; i++)
			{
				int sigStart = (int)((scanlineLength * x) + (chCount * (12.0/scanlineIncPerSample)) + (((double)i * (double)signal.len) / ((double)fieldScanlines)));
				double sampleCounter = 0.0;
				unsigned short glyphPart = glyph[i - startY];
				while (sampleCounter < 12.0)
				{
					int glyphInd0 = (int)sampleCounter;
					int glyphInd1 = glyphInd0 + 1;
					float samplePart = (float)(sampleCounter - (double)glyphInd0);
					float mSamplePart = 1.0f - samplePart;
					unsigned short glyphPart0 = glyphPart >> (15 - glyphInd0);
					glyphPart0 &= 0x01;
					unsigned short glyphPart1 = glyphPart >> (15 - glyphInd1);
					glyphPart1 &= 0x01;
					float inSig = sig[sigStart];
					float inSig0 = inSig;
					float inSig1 = inSig;
					if (glyphPart0) inSig0 = 1.0f;
					if (glyphPart1) inSig1 = 1.0f;
					sig[sigStart] = mSamplePart * inSig0 + samplePart * inSig1;
					sigStart++;
					sampleCounter += scanlineIncPerSample;
				}
			}
			chCount++;
		}
		curCh = *text++;
	}

	return signal;
}
