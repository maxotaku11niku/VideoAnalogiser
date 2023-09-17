/*
* VideoAnalogiser - Command Line Utility for Analogising Digital Videos
* Maxim Hoxha 2023
* Main conversion code
* This software uses code of FFmpeg (http://ffmpeg.org) licensed under the LGPLv2.1 (http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html)
*/

#include <string>
#include <iostream>
#include <random>
#include "ConversionEngine.h"

extern "C"
{
#include "ffmpeg/libavutil/imgutils.h"
#include "ffmpeg/libavutil/opt.h"
}

const char fillChars[5] = { ' ', '\xB0', '\xB1', '\xB2', '\xDB' };

ConversionEngine::ConversionEngine(BroadcastSystems bSys, ColourSystems cSys, double resonance, double prefilterMult, double phaseNoise, double scanlineJitter, double noiseExponent)
{
	//Assumes interlacing for now.
	switch (cSys)
	{
	default:
	case ColourSystems::PAL:
		analogueEnc = new PALSystem(bSys, true, resonance, prefilterMult, phaseNoise, scanlineJitter, noiseExponent);
		break;
	case ColourSystems::NTSC:
		analogueEnc = new NTSCSystem(bSys, true, resonance, prefilterMult, phaseNoise, scanlineJitter, noiseExponent);
		break;
	case ColourSystems::SECAM:
		analogueEnc = new SECAMSystem(bSys, true, resonance, prefilterMult, phaseNoise, scanlineJitter, noiseExponent);
		break;
	}
	actualFramerate = analogueEnc->bcParams->framerate;
	actualFrametime = analogueEnc->bcParams->frameTime;
	alreadyOpen = false;
	outHeight = analogueEnc->bcParams->videoScanlines;
	int vsc = analogueEnc->bcParams->videoScanlines;
	analogueFrameBuffer = new int[vsc * FIXEDWIDTH];
}

//Sets up our converter upon loading a video file
void ConversionEngine::OpenForDecodeVideo(const char* inFileName)
{
	//Open video file
	if (alreadyOpen) CloseDecoder();
	int res = avformat_open_input(&infmtcontext, inFileName, NULL, NULL);
	avformat_find_stream_info(infmtcontext, NULL);

	//Locate video stream
	vidstreamIndex = av_find_best_stream(infmtcontext, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
	invidstream = infmtcontext->streams[vidstreamIndex];
	const AVCodec* ac = avcodec_find_decoder(invidstream->codecpar->codec_id);
	const AVCodecHWConfig* hwconfig;
	invidcodcontext = avcodec_alloc_context3(ac);
	avcodec_parameters_to_context(invidcodcontext, invidstream->codecpar);
	avcodec_open2(invidcodcontext, ac, NULL);
	inWidth = invidcodcontext->width;
	inHeight = invidcodcontext->height;
	inPixFormat = invidcodcontext->pix_fmt;
	vidorigBufsize = av_image_alloc(vidorigData, vidorigLineSize, inWidth, inHeight, inPixFormat, 1);
	incurFrame = av_frame_alloc();
	incurPacket = av_packet_alloc();
	innextPacket = av_packet_alloc();
	innumFrames = invidstream->nb_frames;

	//Locate audio stream
	audstreamIndex = av_find_best_stream(infmtcontext, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
	if (audstreamIndex != AVERROR_STREAM_NOT_FOUND)
	{
		inaudstream = infmtcontext->streams[audstreamIndex];
		ac = avcodec_find_decoder(inaudstream->codecpar->codec_id);
		inaudcodcontext = avcodec_alloc_context3(ac);
		avcodec_parameters_to_context(inaudcodcontext, inaudstream->codecpar);
		avcodec_open2(inaudcodcontext, ac, NULL);
		inAudioRate = inaudcodcontext->sample_rate;
		inAudFormat = inaudcodcontext->sample_fmt;
		inChLayout = inaudcodcontext->ch_layout;
	}

	//Setup rescalers
	inAspect = ((double)inWidth) / ((double)inHeight);
	outWidth = (int)((double)outHeight * inAspect * 0.5) * 2;
	scalercontextForAnalogue = sws_getContext(inWidth, inHeight, inPixFormat, FIXEDWIDTH, outHeight, AVPixelFormat::AV_PIX_FMT_BGRA, SWS_BILINEAR, NULL, NULL, NULL);
	scalercontextForFinal = sws_getContext(FIXEDWIDTH, outHeight, AVPixelFormat::AV_PIX_FMT_BGRA, outWidth, outHeight, AVPixelFormat::AV_PIX_FMT_YUV422P, SWS_BILINEAR, NULL, NULL, NULL);
	vidscaleBufsizeForAnalogue = av_image_alloc(vidscaleDataForAnalogue, vidscaleLineSizeForAnalogue, FIXEDWIDTH, outHeight, AVPixelFormat::AV_PIX_FMT_BGRA, 1);
	vidscaleBufsizeForInterlace = av_image_alloc(vidscaleDataForInterlace, vidscaleLineSizeForInterlace, FIXEDWIDTH, outHeight, AVPixelFormat::AV_PIX_FMT_BGRA, 1);
	vidscaleBufsizeForFinal = av_image_alloc(vidscaleDataForFinal, vidscaleLineSizeForFinal, outWidth, outHeight, AVPixelFormat::AV_PIX_FMT_YUV422P, 1);
	alreadyOpen = true;
	totalTime = (((double)(invidstream->duration)) * ((double)invidstream->time_base.num)) / ((double)invidstream->time_base.den);
}

//This function is not filled yet, which doesn't matter too much for a simple console application (since all resources are freed upon program termination), but WOULD matter in a GUI application.
void ConversionEngine::CloseDecoder()
{
	sws_freeContext(scalercontextForAnalogue);
	sws_freeContext(scalercontextForFinal);
	avcodec_free_context(&invidcodcontext);
	avcodec_free_context(&inaudcodcontext);
	avformat_close_input(&infmtcontext);
}

char* ConversionEngine::GenerateTextProgressBar(double progress, int fullLength)
{
	if (progress < 0) progress = 0;
	char progBarChars[256];
	memset(progBarChars, 0, 256);
	double logicalLength = fullLength * progress;
	int filledLength = (int)logicalLength;
	double partFill = logicalLength - filledLength;
	for (int i = 0; i < filledLength; i++)
	{
		progBarChars[i] = '\xDB';
	}
	if (progress >= 1.0) return progBarChars;
	progBarChars[filledLength] = fillChars[(int)round((partFill * 4.0) + 0.5)];
	for (int i = filledLength + 1; i < fullLength; i++)
	{
		progBarChars[i] = ' ';
	}
	return progBarChars;
}

void ConversionEngine::EncodeVideo(const char* outFileName, bool preview, double kbps, double noise, double crosstalk)
{
	//Zero out the buffer just in case
	for (int i = 0; i < FIXEDWIDTH * outHeight; i++)
	{
		analogueFrameBuffer[i] = 0xFF000000;
	}

	//Setup output video stream
	avformat_alloc_output_context2(&outfmtcontext, NULL, NULL, outFileName);
	const AVOutputFormat* ofmt = outfmtcontext->oformat;
	const AVCodec* ocod = avcodec_find_encoder(ofmt->video_codec);
	outcurPacket = av_packet_alloc();
	outvidstream = avformat_new_stream(outfmtcontext, NULL);
	outvidstream->id = outfmtcontext->nb_streams - 1;
	outvidcodcontext = avcodec_alloc_context3(ocod);
	outvidcodcontext->codec_id = ofmt->video_codec;
	outvidcodcontext->bit_rate = (int64_t)(kbps * 1024.0);
	outvidcodcontext->width = outWidth;
	outvidcodcontext->height = outHeight;
	outvidstream->time_base = analogueEnc->bcParams->ratFrametime;
	outvidcodcontext->time_base = analogueEnc->bcParams->ratFrametime;
	outvidcodcontext->gop_size = 12;
	outvidcodcontext->pix_fmt = AVPixelFormat::AV_PIX_FMT_YUV422P; //just use this so that the output is lossless
	if (outfmtcontext->oformat->flags & AVFMT_GLOBALHEADER) outvidcodcontext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

	//Setup output audio stream
	const AVCodec* acod = avcodec_find_encoder(ofmt->audio_codec);
	outaudPacket = av_packet_alloc();
	outaudstream = avformat_new_stream(outfmtcontext, NULL);
	outaudstream->id = outfmtcontext->nb_streams - 1;
	outaudcodcontext = avcodec_alloc_context3(acod);
	outaudcodcontext->codec_id = ofmt->audio_codec;
	outaudcodcontext->bit_rate = 256000;
	outaudcodcontext->sample_rate = inAudioRate;
	outaudcodcontext->sample_fmt = AVSampleFormat::AV_SAMPLE_FMT_FLTP;
	outlayout = AV_CHANNEL_LAYOUT_STEREO;
	av_channel_layout_copy(&outaudcodcontext->ch_layout, &outlayout);
	outaudstream->time_base = analogueEnc->bcParams->ratFrametime;

	//Setup some other parameters
	AVDictionary* opt = NULL;
	avcodec_open2(outvidcodcontext, ocod, &opt);
	outcurFrame = av_frame_alloc();
	outcurFrame->format = AVPixelFormat::AV_PIX_FMT_YUV422P;
	outcurFrame->width = outWidth;
	outcurFrame->height = outHeight;
	av_frame_get_buffer(outcurFrame, 0);
	avcodec_parameters_from_context(outvidstream->codecpar, outvidcodcontext);
	avcodec_open2(outaudcodcontext, acod, &opt);
	outaudFrame = av_frame_alloc();
	outaudFrame->format = AVSampleFormat::AV_SAMPLE_FMT_FLTP;
	av_channel_layout_copy(&outaudFrame->ch_layout, &outlayout);
	outaudFrame->sample_rate = outaudcodcontext->sample_rate;
	outaudFrame->nb_samples = outaudcodcontext->frame_size;
	av_frame_get_buffer(outaudFrame, 0);
	avcodec_parameters_from_context(outaudstream->codecpar, outaudcodcontext);

	//Setup resampler
	resamplercontext = swr_alloc();
	av_opt_set_chlayout(resamplercontext, "in_chlayout", &inChLayout, 0);
	av_opt_set_int(resamplercontext, "in_sample_rate", inAudioRate, 0);
	av_opt_set_sample_fmt(resamplercontext, "in_sample_fmt", inAudFormat, 0);
	av_opt_set_chlayout(resamplercontext, "out_chlayout", &outaudFrame->ch_layout, 0);
	av_opt_set_int(resamplercontext, "out_sample_rate", outaudcodcontext->sample_rate, 0);
	av_opt_set_sample_fmt(resamplercontext, "out_sample_fmt", AVSampleFormat::AV_SAMPLE_FMT_FLTP, 0);
	swr_init(resamplercontext);

	//Open output filestream
	avio_open(&outfmtcontext->pb, outFileName, AVIO_FLAG_WRITE);
	avformat_write_header(outfmtcontext, &opt);
	av_dict_free(&opt);

	//Initialise loop
	av_seek_frame(infmtcontext, vidstreamIndex, 0, 0);
	av_read_frame(infmtcontext, incurPacket);
	avcodec_send_packet(invidcodcontext, incurPacket);
	avcodec_receive_frame(invidcodcontext, incurFrame);
	av_image_copy(vidorigData, vidorigLineSize, (const unsigned char**)incurFrame->data, incurFrame->linesize, inPixFormat, inWidth, inHeight);
	sws_scale(scalercontextForAnalogue, vidorigData, vidorigLineSize, 0, inHeight, vidscaleDataForAnalogue, vidscaleLineSizeForAnalogue);
	int interlaceField = 0;
	int numTransSamp = 0;
	int totalSamp = 0;
	int64_t curFrame = 0;
	SignalPack sig;
	FrameData finData;
	double curTime = 0.0;
	double refTime = 0.0;
	int totalNumFrames;
	if (preview) totalNumFrames = 300;
	else totalNumFrames = (int)((((double)(invidstream->duration)) * ((double)invidstream->time_base.num)) / ((double)invidstream->time_base.den * actualFrametime));
	std::mt19937_64 rng;
	std::uniform_real_distribution<> ndist(-noise, noise);
	char progString[256];
	for (int i = 0; i < totalNumFrames; i++)
	{
		av_frame_make_writable(outcurFrame);
		sig = analogueEnc->Encode({ (int*)vidscaleDataForAnalogue[0], FIXEDWIDTH, outHeight }, interlaceField);
		for (int j = 0; j < sig.len; j++) //Will be replaced with a generic signal transform function soon
		{
			sig.signal[j] += ndist(rng);
		}
		finData = analogueEnc->Decode(sig, interlaceField, crosstalk);
		for (int j = 0; j < analogueEnc->bcParams->videoScanlines / 2; j++) //We only got half a frame out, so we assume interlacing and copy appropriately
		{
			memcpy(analogueFrameBuffer + (finData.width * (j * 2 + interlaceField)), finData.image + (finData.width * j), finData.width * 4);
		}
		//Note that the video is technically progressive scan, but we force interlacing through the content to support codecs which don't allow interlaced scan
		memcpy(vidscaleDataForInterlace[0], analogueFrameBuffer, finData.width * outHeight * 4);
		sws_scale(scalercontextForFinal, vidscaleDataForInterlace, vidscaleLineSizeForInterlace, 0, outHeight, vidscaleDataForFinal, vidscaleLineSizeForFinal);
		av_image_copy(outcurFrame->data, outcurFrame->linesize, (const unsigned char**)vidscaleDataForFinal, vidscaleLineSizeForFinal, AVPixelFormat::AV_PIX_FMT_YUV422P, outWidth, outHeight);
		outcurFrame->pts = curFrame++;
		avcodec_send_frame(outvidcodcontext, outcurFrame);
		avcodec_receive_packet(outvidcodcontext, outcurPacket);
		av_packet_rescale_ts(outcurPacket, outvidcodcontext->time_base, outvidstream->time_base);
		outcurPacket->stream_index = outvidstream->index;
		av_interleaved_write_frame(outfmtcontext, outcurPacket);

		curTime = actualFrametime * (i + 1);

		while (refTime < curTime)
		{
			av_packet_unref(incurPacket);
			av_read_frame(infmtcontext, incurPacket);
			if (incurPacket->data == nullptr)
			{
				break;
			}
			if (incurPacket->stream_index == vidstreamIndex)
			{
				avcodec_send_packet(invidcodcontext, incurPacket);
				avcodec_receive_frame(invidcodcontext, incurFrame);
				refTime = (((double)incurPacket->dts) * ((double)invidstream->time_base.num)) / ((double)invidstream->time_base.den);
				if (incurFrame->data[0] != NULL)
				{
					av_image_copy(vidorigData, vidorigLineSize, (const unsigned char**)incurFrame->data, incurFrame->linesize, inPixFormat, inWidth, inHeight);
					sws_scale(scalercontextForAnalogue, vidorigData, vidorigLineSize, 0, inHeight, vidscaleDataForAnalogue, vidscaleLineSizeForAnalogue);
				}
			}
			else if (incurPacket->stream_index == audstreamIndex)
			{
				avcodec_send_packet(inaudcodcontext, incurPacket);
				avcodec_receive_frame(inaudcodcontext, incurFrame);
				if (incurFrame->data[0] != NULL)
				{
					av_frame_make_writable(outaudFrame);
					numTransSamp = av_rescale_rnd(swr_get_delay(resamplercontext, outaudcodcontext->sample_rate) + incurFrame->nb_samples, outaudcodcontext->sample_rate, outaudcodcontext->sample_rate, AV_ROUND_UP);
					swr_convert(resamplercontext, outaudFrame->data, numTransSamp, (const unsigned char**)incurFrame->data, incurFrame->nb_samples);
					outaudFrame->pts = av_rescale_q(totalSamp, { 1, outaudcodcontext->sample_rate }, outaudcodcontext->time_base);
					totalSamp += numTransSamp;
					avcodec_send_frame(outaudcodcontext, outaudFrame);
					avcodec_receive_packet(outaudcodcontext, outaudPacket);
					av_packet_rescale_ts(outaudPacket, outaudcodcontext->time_base, outaudstream->time_base);
					outaudPacket->stream_index = outaudstream->index;
					av_interleaved_write_frame(outfmtcontext, outaudPacket);
				}
			}
		}
		interlaceField = interlaceField ? 0 : 1;
		delete[] sig.signal;
		delete[] finData.image;
		sprintf(progString, "Wrote frame %u/%u\n", i + 1, totalNumFrames);
		strcat(progString, "[");
		strcat(progString, GenerateTextProgressBar(((double)(i + 1)) / ((double)totalNumFrames), 78));
		strcat(progString, "]");
		std::cout << progString << "\x1B[1F";
	}
	std::cout << std::endl;

	//Write epilogue and finish
	av_write_trailer(outfmtcontext);
	avcodec_free_context(&outvidcodcontext);
	av_frame_free(&outcurFrame);
	av_packet_free(&outcurPacket);
	avio_closep(&outfmtcontext->pb);
	avformat_free_context(outfmtcontext);
}