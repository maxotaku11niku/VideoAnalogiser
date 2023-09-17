/*
* VideoAnalogiser - Command Line Utility for Analogising Digital Videos
* Maxim Hoxha 2023
* SECAM encoder/decoder
* This software uses code of FFmpeg (http://ffmpeg.org) licensed under the LGPLv2.1 (http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html)
*/

#include <iostream>
#include "SECAMSystem.h"

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

    DbSignalI = new float[signalLen];
    DbSignalQ = new float[signalLen];
    DrSignalI = new float[signalLen];
    DrSignalQ = new float[signalLen];

    jitGen = new MultiOctaveNoiseGen(11, 0.0, scanlineJitter * activeWidth, noiseExponent);
    phNoiseGen = new MultiOctaveNoiseGen(11, 0.0, phaseNoise, noiseExponent);
}

SignalPack SECAMSystem::Encode(FrameData imgdat, int interlaceField)
{
    double realActiveTime = bcParams->activeTime;
    double realScanlineTime = 1.0 / (double)(fieldScanlines * bcParams->framerate);
    int signalLen = (int)(imgdat.width * fieldScanlines * (realScanlineTime / realActiveTime)); //To get a good analogue feel, we must limit the vertical resolution; the horizontal resolution will be limited as we decode the distorted signal.
    float* signalOut = new float[signalLen];
    double R = 0.0;
    double G = 0.0;
    double B = 0.0;
    double Db = 0.0;
    double Dr = 0.0;
    double time = 0;
    int pos = 0;
    int polarity = 0;
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

    double instantPhase = 0.0;
    int* imgColours = imgdat.image;
    int currentScanline;
    double finSamp = 0.0;
    int w = imgdat.width;
    int col;
    int subcarrierstartind = (int)((SUBCARRIER_START_TIME / realActiveTime) * ((double)imgdat.width));
    SignalPack Ysig = { new float[signalLen], signalLen };
    SignalPack Dbsig = { new float[signalLen], signalLen };
    SignalPack Drsig = { new float[signalLen], signalLen };
    //Make component signals
    for (int i = 0; i < fieldScanlines; i++) //Only generate active scanlines
    {
        currentScanline = interlaced ? (i * 2 + polarity) % bcParams->videoScanlines : i;
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
    SignalPack filtYsig = ApplyFIRFilter(Ysig, lumaprefir);
    SignalPack filtDbsig = ApplyFIRFilter(Dbsig, chromaprefir);
    SignalPack filtDrsig = ApplyFIRFilter(Drsig, chromaprefir);
    pos = 0;
    double scAngFreq = 0.0;
    double scAngFreqShift = 0.0;
    float* curChromaSig;
    //Composite component signals
    for (int i = 0; i < fieldScanlines; i++)
    {
        if ((i % 2) == 1) //SECAM alternates between Db and Dr with each scanline
        {
            scAngFreq = bcParams->carrierAngFreqDr;
            scAngFreqShift = bcParams->deltaAngFreqDr;
            curChromaSig = filtDrsig.signal;
        }
        else
        {
            scAngFreq = bcParams->carrierAngFreqDb;
            scAngFreqShift = bcParams->deltaAngFreqDb;
            curChromaSig = filtDbsig.signal;
        }
        for (int j = 0; j < subcarrierstartind; j++)
        {
            signalOut[pos] = 0.0;
        }
        instantPhase = 0.0;
        while (pos < boundaryPoints[i + 1])
        {
            instantPhase += sampleTime * (scAngFreq + scAngFreqShift * (double)curChromaSig[pos]);
            signalOut[pos] = filtYsig.signal[pos] + 0.115 * cos(instantPhase); //Add chroma via FM
            pos++;
        }
    }

    delete[] Ysig.signal;
    delete[] Dbsig.signal;
    delete[] Drsig.signal;
    delete[] filtYsig.signal;
    delete[] filtDbsig.signal;
    delete[] filtDrsig.signal;

    return { signalOut, signalLen };
}

FrameData SECAMSystem::Decode(SignalPack signal, int interlaceField, double crosstalk)
{
    double realActiveTime = bcParams->activeTime;
    double realScanlineTime = 1.0 / (double)(fieldScanlines * bcParams->framerate);
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
    
    /*/
    std::uniform_real_distribution<> phdist(-phaseNoise, phaseNoise);
    double time = 0.0;
    double phOffs = 0.0;
    double scAngFreqDb = bcParams->carrierAngFreqDb;
    double scAngFreqDr = bcParams->carrierAngFreqDr;
    for (int i = 0; i < fieldScanlines; i++)
    {
        phOffs = phdist(rng);
        phOffs += phaseError;
        while (pos < boundaryPoints[i + 1])
        {
            time = pos * sampleTime;
            DbSignalI[pos] = DbSignal.signal[pos] * sin(scAngFreqDb * time + phOffs);
            DbSignalQ[pos] = DbSignal.signal[pos] * cos(scAngFreqDb * time + phOffs);
            DrSignalI[pos] = DrSignal.signal[pos] * sin(scAngFreqDr * time + phOffs);
            DrSignalQ[pos] = DrSignal.signal[pos] * cos(scAngFreqDr * time + phOffs);
            pos++;
        }
    }

    SignalPack filtDbSignalI = ApplyFIRFilter({ DbSignalI, signal.len }, dbfir);
    SignalPack filtDbSignalQ = ApplyFIRFilter({ DbSignalQ, signal.len }, dbfir);
    SignalPack filtDrSignalI = ApplyFIRFilter({ DrSignalI, signal.len }, drfir);
    SignalPack filtDrSignalQ = ApplyFIRFilter({ DrSignalQ, signal.len }, drfir);

    double diffI = 0.0;
    double diffQ = 0.0;
    for (int i = 1; i < signal.len; i++)
    {
        diffI = filtDbSignalI.signal[i] - filtDbSignalI.signal[i - 1];
        diffQ = filtDbSignalQ.signal[i] - filtDbSignalQ.signal[i - 1];
        DbSignal.signal[i] = sqrt(diffI * diffI + diffQ * diffQ) * 100.0;
        diffI = filtDrSignalI.signal[i] - filtDrSignalI.signal[i - 1];
        diffQ = filtDrSignalQ.signal[i] - filtDrSignalQ.signal[i - 1];
        DrSignal.signal[i] = sqrt(diffI * diffI + diffQ * diffQ) * 100.0;
    }
    //*/

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
    for (int i = 0; i < signal.len; i++) //Somehow this bunch of magic acts as a functional FM decoder
    {
        DbDeriv = (double)DbSignal.signal[i] - DbLast;
        DrDeriv = (double)DrSignal.signal[i] - DrLast;
        DbLast = DbSignal.signal[i];
        DrLast = DrSignal.signal[i];
        curDb = (DbDecodeAngFreq - scAngFreqDb) / scAngFreqShiftDb;
        curDr = (DrDecodeAngFreq - scAngFreqDr) / scAngFreqShiftDr;
        DbSignal.signal[i] = curDb;
        DrSignal.signal[i] = curDr;
        DbFreqShift = -(cos(DbDecodePhase) * DbDeriv) - (DbDecodeAngFreq * sin(DbDecodePhase) * DbLast);
        DrFreqShift = -(cos(DrDecodePhase) * DrDeriv) - (DrDecodeAngFreq * sin(DrDecodePhase) * DrLast);
        DbDecodeAngFreq += 1.1 * (DbFreqShift - DbLastFreqShift);
        DrDecodeAngFreq += 1.1 * (DrFreqShift - DrLastFreqShift);
        DbDecodePhase += sampleTime * DbDecodeAngFreq;
        DrDecodePhase += sampleTime * DrDecodeAngFreq;
        DbLastFreqShift = DbFreqShift;
        DrLastFreqShift = DrFreqShift;
    }
    //*/

    SignalPack finalSignal = ApplyFIRFilterNotchCrosstalkShift(newSignal, colfir, crosstalk, sampleTime, bcParams->carrierAngFreq);
    SignalPack finalDbSignal = ApplyFIRFilter(DbSignal, dbfir);
    SignalPack finalDrSignal = ApplyFIRFilter(DrSignal, drfir);

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

    /*/
    delete[] filtDbSignalI.signal;
    delete[] filtDbSignalQ.signal;
    delete[] filtDrSignalI.signal;
    delete[] filtDrSignalQ.signal;
    //*/
    delete[] DbSignal.signal;
    delete[] DrSignal.signal;
    delete[] newSignal.signal;
    delete[] finalSignal.signal;
    delete[] finalDbSignal.signal;
    delete[] finalDrSignal.signal;

    return writeToSurface;
}