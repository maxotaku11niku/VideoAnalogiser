/*
* VideoAnalogiser - Command Line Utility for Analogising Digital Videos
* Maxim Hoxha 2023-2026
* SECAM encoder/decoder
* This software uses code of FFmpeg (http://ffmpeg.org) licensed under the LGPLv2.1 (http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html)
*/

#include <iostream>
#include "SECAMSystem.h"
#include "VHSFont.h"

SECAMSystem::SECAMSystem(BroadcastSystems sys, bool interlace, double resonance, double prefilterMult, double phaseNoise, double scanlineJitter, double noiseExponent)
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

	RGBtoYCCConversionMatrix = RGBtoYDbDrConversionMatrix;
	YCCtoRGBConversionMatrix = YDbDrtoRGBConversionMatrix;

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
    dbfir = MakeFIRFilter(sampleRate, 256, (bcParams->chromaBandwidthUpperDb - bcParams->chromaBandwidthLowerDb) / 2.0, bcParams->chromaBandwidthLowerDb + bcParams->chromaBandwidthUpperDb, resonance);
    drfir = MakeFIRFilter(sampleRate, 256, (bcParams->chromaBandwidthUpperDr - bcParams->chromaBandwidthLowerDr) / 2.0, bcParams->chromaBandwidthLowerDr + bcParams->chromaBandwidthUpperDr, resonance);
    colfir = MakeFIRFilter(sampleRate, 256, (bcParams->chromaBandwidthUpper - bcParams->chromaBandwidthLower) / 2.0, bcParams->chromaBandwidthLower + bcParams->chromaBandwidthUpper, resonance);

    std::cout << "Creating prefilters..." << std::endl;

    lumaprefir = MakeFIRFilter(sampleRate, 256, 0.0, 2.0 * bcParams->mainBandwidth * prefilterMult, PREFILTER_RESONANCE);
    chromaprefir = MakeFIRFilter(sampleRate, 256, 0.0, 2.0 * bcParams->chromaBandwidthLower * prefilterMult, PREFILTER_RESONANCE);

    jitGen = new MultiOctaveNoiseGen(11, 0.0, scanlineJitter * activeWidth, noiseExponent);
    phNoiseGen = new MultiOctaveNoiseGen(11, 0.0, phaseNoise, noiseExponent);
}

SignalPack SECAMSystem::Encode(FrameData imgdat, int field)
{
    double realActiveTime = bcParams->activeTime;
    double realScanlineTime = bcParams->scanlineTime;
    int signalLen = (int)(imgdat.width * fieldScanlines * (realScanlineTime / realActiveTime)); //To get a good analogue feel, we must limit the vertical resolution; the horizontal resolution will be limited as we decode the distorted signal.
    float* signalOut = new float[signalLen];
    double R = 0.0;
    double G = 0.0;
    double B = 0.0;
    double Db = 0.0;
    double Dr = 0.0;
    double time = 0;
    int pos = 0;
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
    int subcarrierstartind = (int)((SUBCARRIER_START_TIME / realActiveTime) * ((double)imgdat.width));
    SignalPack Ysig = { new float[signalLen], signalLen };
    SignalPack Dbsig = { new float[signalLen], signalLen };
    SignalPack Drsig = { new float[signalLen], signalLen };
    //Make component signals
    for (int i = 0; i < fieldScanlines; i++) //Only generate active scanlines
    {
        currentScanline = interlaced ? (i * 2 + interlaceField) % bcParams->videoScanlines : i;
        for (int j = 0; j < activeSignalStarts[i]; j++) //Front porch, ignore sync signal because we don't see its results
        {
            Ysig.signal[pos] = 0.0f;
            Dbsig.signal[pos] = 0.0f;
            Drsig.signal[pos] = 0.0f;
            pos++;
        }
        for (int j = 0; j < imgdat.width; j++) //Active signal
        {
            col = imgColours[currentScanline * w + j];
            R = ((col & 0x00FF0000) >> 16) / 255.0;
            G = ((col & 0x0000FF00) >> 8) / 255.0;
            B = (col & 0x000000FF) / 255.0;
            R = SRGBGammaTransform(R); //SRGB correction
            G = SRGBGammaTransform(G);
            B = SRGBGammaTransform(B);
            R = pow(R, 1.0 / 2.8); //Gamma correction
            G = pow(G, 1.0 / 2.8);
            B = pow(B, 1.0 / 2.8);
            Ysig.signal[pos] = RGBtoYDbDrConversionMatrix[0] * R + RGBtoYDbDrConversionMatrix[1] * G + RGBtoYDbDrConversionMatrix[2] * B; //Encode Y, Db and Dr
            Dbsig.signal[pos] = RGBtoYDbDrConversionMatrix[3] * R + RGBtoYDbDrConversionMatrix[4] * G + RGBtoYDbDrConversionMatrix[5] * B;
            Drsig.signal[pos] = RGBtoYDbDrConversionMatrix[6] * R + RGBtoYDbDrConversionMatrix[7] * G + RGBtoYDbDrConversionMatrix[8] * B;
            pos++;
        }
        while (pos < boundaryPoints[i + 1]) //Back porch, ignore sync signal because we don't see its results
        {
            Ysig.signal[pos] = 0.0f;
            Dbsig.signal[pos] = 0.0f;
            Drsig.signal[pos] = 0.0f;
            pos++;
        }
    }

    //Prefilter signals
    SignalPack filtYsig1 = ApplyFIRFilter(Ysig, lumaprefir);
    SignalPack filtYsig2 = ApplyFIRFilterNotchShift(filtYsig1, chromaprefir, sampleTime, bcParams->carrierAngFreq);
    SignalPack filtDbsig = ApplyFIRFilter(Dbsig, chromaprefir);
    SignalPack filtDrsig = ApplyFIRFilter(Drsig, chromaprefir);
    pos = 0;
    float* curChromaSig;
    double instantPhaseDb = 0.0;
    double instantPhaseDr = 0.0;
    double scAngFreqDb = bcParams->carrierAngFreqDb;
    double scAngFreqShiftDb = bcParams->deltaAngFreqDb;
    double scAngFreqDr = bcParams->carrierAngFreqDr;
    double scAngFreqShiftDr = bcParams->deltaAngFreqDr;
    //Composite component signals
    for (int i = 0; i < fieldScanlines; i++)
    {
        if ((i % 2) == 1) //SECAM alternates between Db and Dr with each scanline
        {
            curChromaSig = filtDrsig.signal;
            for (int j = 0; j < subcarrierstartind; j++)
            {
                signalOut[pos] = filtYsig2.signal[pos];
                instantPhaseDr += sampleTime * scAngFreqDr;
                pos++;
            }
            while (pos < boundaryPoints[i + 1])
            {
                signalOut[pos] = filtYsig2.signal[pos] + 0.115 * cos(instantPhaseDr); //Add chroma via FM
                instantPhaseDr += sampleTime * (scAngFreqDr + scAngFreqShiftDr * (double)curChromaSig[pos]);
                pos++;
            }
            instantPhaseDb = fmod(instantPhaseDr, 2.0 * M_PI);
        }
        else
        {
            curChromaSig = filtDbsig.signal;
            for (int j = 0; j < subcarrierstartind; j++)
            {
                signalOut[pos] = filtYsig2.signal[pos];
                instantPhaseDb += sampleTime * scAngFreqDb;
                pos++;
            }
            while (pos < boundaryPoints[i + 1])
            {
                signalOut[pos] = filtYsig2.signal[pos] + 0.115 * cos(instantPhaseDb); //Add chroma via FM
                instantPhaseDb += sampleTime * (scAngFreqDb + scAngFreqShiftDb * (double)curChromaSig[pos]);
                pos++;
            }
            instantPhaseDb = fmod(instantPhaseDb, 2.0 * M_PI);
        }
    }

    delete[] Ysig.signal;
    delete[] Dbsig.signal;
    delete[] Drsig.signal;
    delete[] filtYsig1.signal;
    delete[] filtYsig2.signal;
    delete[] filtDbsig.signal;
    delete[] filtDrsig.signal;

    return { signalOut, signalLen };
}

FrameData SECAMSystem::Decode(SignalPack signal, int field, double crosstalk)
{
    double realActiveTime = bcParams->activeTime;
    double realScanlineTime = bcParams->scanlineTime;
    double realFrameTime = (interlaced ? 2.0 : 1.0) / bcParams->framerate;
    int R = 0;
    int G = 0;
    int B = 0;
    double Y = 0.0;
    double Db = 0.0;
    double Dr = 0.0;
    int polarity = 0;
    int pos = 0;
    int DbPos = 0;
    int DrPos = 0;
    int componentAlternate = 0; //SECAM alternates between Db and Dr with each scanline
    double sigNum = 0.0;
    double blendStr = 1.0 - crosstalk;
    double freqPoint = 0.0;
    double sampleTime = realActiveTime / (double)activeWidth;
    
    SignalPack DbSignal = ApplyFIRFilterCrosstalkShift(signal, colfir, crosstalk, sampleTime, bcParams->carrierAngFreqDb);
    SignalPack DrSignal = ApplyFIRFilterCrosstalkShift(signal, colfir, crosstalk, sampleTime, bcParams->carrierAngFreqDr);
    SignalPack newSignal = ApplyFIRFilter(signal, mainfir);

    /**/
    //Extract FM colour signals (does anyone have a better way to do this rather than this hacky way?)
    double DbDecodeAngFreq = bcParams->carrierAngFreqDb;
    double DrDecodeAngFreq = bcParams->carrierAngFreqDr;
    double scAngFreqDb = bcParams->carrierAngFreqDb;
    double scAngFreqDr = bcParams->carrierAngFreqDr;
    double scAngFreqShiftDb = bcParams->deltaAngFreqDb;
    double scAngFreqShiftDr = bcParams->deltaAngFreqDr;
    double DbDecodePhase = 0.0;
    double DrDecodePhase = 0.0;
    double DbDeriv = 0.0;
    double DrDeriv = 0.0;
    double DbLast = 0.0;
    double DrLast = 0.0;
    double curDb = 0.0;
    double curDr = 0.0;
    double DbFreqShift = 0.0;
    double DrFreqShift = 0.0;
    double DbLastFreqShift = 0.0;
    double DrLastFreqShift = 0.0;
    SignalPack DbDecodedSignal = { new float[signal.len], signal.len };
    SignalPack DrDecodedSignal = { new float[signal.len], signal.len };
    for (int i = 0; i < signal.len; i++) //Somehow this bunch of magic acts as a functional FM decoder
    {
        DbLast = DbSignal.signal[i - 1];
        DrLast = DrSignal.signal[i - 1];
        DbDeriv = (double)DbSignal.signal[i] - DbLast;
        DrDeriv = (double)DrSignal.signal[i] - DrLast;
        curDb = (DbDecodeAngFreq - scAngFreqDb) / scAngFreqShiftDb;
        curDr = (DrDecodeAngFreq - scAngFreqDr) / scAngFreqShiftDr;
        DbDecodedSignal.signal[i] = curDb;
        DrDecodedSignal.signal[i] = curDr;
        DbFreqShift = -(cos(DbDecodePhase) * DbDeriv) - (DbDecodeAngFreq * sin(DbDecodePhase) * DbSignal.signal[i]);
        DrFreqShift = -(cos(DrDecodePhase) * DrDeriv) - (DrDecodeAngFreq * sin(DrDecodePhase) * DrSignal.signal[i]);
        DbDecodeAngFreq += 1.5 * (DbFreqShift - DbLastFreqShift);
        DrDecodeAngFreq += 1.5 * (DrFreqShift - DrLastFreqShift);
        DbDecodePhase += sampleTime * DbDecodeAngFreq;
        DrDecodePhase += sampleTime * DrDecodeAngFreq;
        DbLastFreqShift = DbFreqShift;
        DrLastFreqShift = DrFreqShift;
    }
    //*/

    SignalPack finalSignal = ApplyFIRFilterNotchCrosstalkShift(newSignal, colfir, crosstalk, sampleTime, bcParams->carrierAngFreq);
    SignalPack finalDbSignal = ApplyFIRFilter(DbDecodedSignal, dbfir);
    SignalPack finalDrSignal = ApplyFIRFilter(DrDecodedSignal, drfir);

    FrameData writeToSurface = { new int[activeWidth * fieldScanlines], activeWidth, fieldScanlines };
    for (int i = 0; i < fieldScanlines; i++) //Where the active signal starts
    {
        activeSignalStarts[i] = (int)((((double)i * (double)signal.len) / (double)fieldScanlines) + ((realScanlineTime - realActiveTime) / (2 * realActiveTime)) * activeWidth);
    }

    int* surfaceColours = writeToSurface.image;
    int currentScanline;
    int curjit = 0;
    int finCol = 0;
    double dR = 0.0;
    double dG = 0.0;
    double dB = 0.0;
    //Write decoded signals to our frame
    for (int i = 0; i < fieldScanlines; i++)
    {
        componentAlternate = i % 2;
        curjit = (int)jitGen->GenNoise();
        if (curjit > 100) curjit = 100;
        if (curjit < -100) curjit = -100; //Limit jitter distance to prevent buffer overflow
        pos = activeSignalStarts[i] + curjit;
        DbPos = activeSignalStarts[componentAlternate == 0 ? i : (i - 1)] + curjit;
        if (i <= 0)
        {
            for (int j = 0; j < writeToSurface.width; j++) //Decode active signal region only
            {
                Y = finalSignal.signal[pos];
                Db = finalDbSignal.signal[DbPos];
                Dr = 0.0;
                dR = pow(YDbDrtoRGBConversionMatrix[0] * Y + YDbDrtoRGBConversionMatrix[2] * Dr, 2.8);
                dG = pow(YDbDrtoRGBConversionMatrix[3] * Y + YDbDrtoRGBConversionMatrix[4] * Db + YDbDrtoRGBConversionMatrix[5] * Dr, 2.8);
                dB = pow(YDbDrtoRGBConversionMatrix[6] * Y + YDbDrtoRGBConversionMatrix[7] * Db, 2.8);
                R = (int)(CD_CLAMP(SRGBInverseGammaTransform(dR), 0.0, 1.0) * 255.0);
                G = (int)(CD_CLAMP(SRGBInverseGammaTransform(dG), 0.0, 1.0) * 255.0);
                B = (int)(CD_CLAMP(SRGBInverseGammaTransform(dB), 0.0, 1.0) * 255.0);
                finCol = 0xFF000000;
                finCol |= R << 16;
                finCol |= G << 8;
                finCol |= B;
                surfaceColours[i * writeToSurface.width + j] = finCol;
                pos++;
                DbPos++;
            }
        }
        else
        {
            DrPos = activeSignalStarts[componentAlternate == 0 ? (i - 1) : i] + curjit;

            for (int j = 0; j < writeToSurface.width; j++) //Decode active signal region only
            {
                Y = finalSignal.signal[pos];
                Db = finalDbSignal.signal[DbPos];
                Dr = finalDrSignal.signal[DrPos];
                dR = pow(YDbDrtoRGBConversionMatrix[0] * Y + YDbDrtoRGBConversionMatrix[2] * Dr, 2.8);
                dG = pow(YDbDrtoRGBConversionMatrix[3] * Y + YDbDrtoRGBConversionMatrix[4] * Db + YDbDrtoRGBConversionMatrix[5] * Dr, 2.8);
                dB = pow(YDbDrtoRGBConversionMatrix[6] * Y + YDbDrtoRGBConversionMatrix[7] * Db, 2.8);
                R = (int)(CD_CLAMP(SRGBInverseGammaTransform(dR), 0.0, 1.0) * 255.0);
                G = (int)(CD_CLAMP(SRGBInverseGammaTransform(dG), 0.0, 1.0) * 255.0);
                B = (int)(CD_CLAMP(SRGBInverseGammaTransform(dB), 0.0, 1.0) * 255.0);
                finCol = 0xFF000000;
                finCol |= R << 16;
                finCol |= G << 8;
                finCol |= B;
                surfaceColours[i * writeToSurface.width + j] = finCol;
                pos++;
                DbPos++;
                DrPos++;
            }
        }
    }

    delete[] DbSignal.signal;
    delete[] DrSignal.signal;
    delete[] DbDecodedSignal.signal;
    delete[] DrDecodedSignal.signal;
    delete[] newSignal.signal;
    delete[] finalSignal.signal;
    delete[] finalDbSignal.signal;
    delete[] finalDrSignal.signal;

    return writeToSurface;
}

//I know this is wrong! (TODO)
SignalPack SECAMSystem::AddText(SignalPack signal, const char* text, double x, int y, bool yRelativeToBottom)
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
