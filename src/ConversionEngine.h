/*
* VideoAnalogiser - Command Line Utility for Analogising Digital Videos
* Maxim Hoxha 2023
* Main conversion code
* This software uses code of FFmpeg (http://ffmpeg.org) licensed under the LGPLv2.1 (http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html)
*/

#pragma once
#include "PALSystem.h"
#include "NTSCSystem.h"
#include "SECAMSystem.h"

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
}

class ConversionEngine
{
public:
	ConversionEngine(BroadcastSystems bSys, ColourSystems cSys, double resonance, double prefilterMult, double phaseNoise, double scanlineJitter, double noiseExponent);

	void OpenForDecodeVideo(const char* inFileName);
    void EncodeVideo(const char* outFileName, bool preview, double kbps, double noise, double crosstalk);
	void CloseDecoder();
private:
    void GenerateTextProgressBar(double progress, int fullLength, char* progBarChars);
    ColourSystem* analogueEnc = NULL;
	AVFormatContext* infmtcontext = NULL;
	AVCodecContext* invidcodcontext = NULL;
	AVCodecContext* inaudcodcontext = NULL;
    SwsContext* scalercontextForAnalogue = NULL;
    SwsContext* scalercontextForFinal = NULL;
    SwrContext* resamplercontext = NULL;
    AVStream* invidstream = NULL;
    AVStream* inaudstream = NULL;
    AVFrame* incurFrame = NULL;
    AVPacket* incurPacket = NULL;
    AVPacket* innextPacket = NULL;
    int vidstreamIndex;
    int audstreamIndex;
    int inWidth;
    int inHeight;
    double inAspect;
    enum AVPixelFormat inPixFormat;
    int inAudioRate;
    int innumFrames;
    AVChannelLayout inChLayout;
    enum AVSampleFormat inAudFormat;
    bool alreadyOpen;

    AVFormatContext* outfmtcontext = NULL;
    AVCodecContext* outvidcodcontext = NULL;
    AVCodecContext* outaudcodcontext = NULL;
    AVStream* outvidstream = NULL;
    AVStream* outaudstream = NULL;
    AVFrame* outcurFrame = NULL;
    AVFrame* outaudFrame = NULL;
    AVPacket* outcurPacket = NULL;
    AVPacket* outaudPacket = NULL;
    float realoutsamplerate;
    int sampleratespec;
    AVSampleFormat outsampformat;
    AVChannelLayout outlayout;
    unsigned char** audorigData;
    unsigned char** audresData;
    int audorigLineSize;
    int audresLineSize;
    short* audfullstreamShort;
    unsigned char* audfullstreamByte;
    int curtotalByteStreamSize;

    unsigned char* vidorigData[4];
    int vidorigLineSize[4];
    int convnumFrames;
    int frameskip;
    int outWidth;
    int outHeight;
    double actualFramerate;
    double actualFrametime;
    double totalTime;
    unsigned char* vidscaleDataForAnalogue[4];
    int vidscaleLineSizeForAnalogue[4];
    int vidorigBufsize;
    int vidscaleBufsizeForAnalogue;
    unsigned char* vidscaleDataForInterlace[4];
    int vidscaleLineSizeForInterlace[4];
    int vidscaleBufsizeForInterlace;
    unsigned char* vidscaleDataForFinal[4];
    int vidscaleLineSizeForFinal[4];
    int vidscaleBufsizeForFinal;
    int* analogueFrameBuffer;
};
