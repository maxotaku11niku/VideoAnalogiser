/*
* VideoAnalogiser - Command Line Utility for Analogising Digital Videos
* Maxim Hoxha 2023
* Entry point and command line parsing
* This software uses code of FFmpeg (http://ffmpeg.org) licensed under the LGPLv2.1 (http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html)
*/

#include "VideoAnalogiser.h"
#include "ConversionEngine.h"

void ShowHelp()
{
	std::cout << "Usage:" << std::endl;
	std::cout << "[input filename] [output filename] <options> OR -h OR -bsyshelp [system]" << std::endl << std::endl;
	std::cout << "Options:" << std::endl;
	std::cout << "-h: Displays this help, then quits." << std::endl;
	std::cout << "-bsys [system]: Use given broadcast system (which will affect framerates and picture quality). Valid values: m n b g h i d k l vhs525 vhs625. Defaults to i for PAL, m for NTSC and l for SECAM." << std::endl;
	std::cout << "-bsyshelp [system]: Dumps parameters relevant to the given broadcast system, then quits." << std::endl;
	std::cout << "-csys [system]: Use given colour system. This option corresponds to the familiar PAL, NTSC and SECAM standards. Valid values: pal, ntsc, secam. Defaults to pal." << std::endl;
	std::cout << "-vhs: Use the appropriate VHS standard for the given colour standard (VHS 525 line for NTSC, VHS 625 line for PAL and SECAM). Overrides -bsys." << std::endl;
	std::cout << "-br [kb/s]: Output video bitrate in kb/s. Make this higher if your output is expected to be pretty noisy, recommended values 5000 - 20000. Defaults to 10000." << std::endl;
	std::cout << "-preview: Generate 300 frames of footage from the start of the video to preview the current settings." << std::endl;
	std::cout << "-noise [amount]: Magnitude of signal noise, recommended values 0.0 - 0.5. Defaults to 0.0." << std::endl;
	std::cout << "-jitter [amount]: Magnitude of scanline jitter, recommended values 0.0 - 0.01. Defaults to 0.0." << std::endl;
	std::cout << "-reso [amount]: Decode filter resonance, recommended values 2.0 - 20.0. Defaults to 5.0." << std::endl;
	std::cout << "-prefreq [amount]: Prefilter width multiplier, recommended values 0.1 - 1.0. Defaults to 0.7." << std::endl;
	std::cout << "-psnoise [amount]: Colour decoder phase noise (per scanline), recommended values 0.0 - 3.0. Defaults to 0.0." << std::endl;
	std::cout << "-crosstalk [amount]: Decoder luma-chroma crosstalk, recommended values 0.0 - 1.0. Defaults to 0.0." << std::endl;
	std::cout << "-pfnoise [amount]: Colour decoder phase noise (per field), recommended values 0.0 - 3.0. Defaults to 0.0." << std::endl;
}

int main(int argc, char** argv)
{
	std::cout << "VideoAnalogiser - Command Line Utility for Analogising Digital Videos" << std::endl;
	std::cout << "Version 0.8" << std::endl;
	std::cout << "Ideal for faking the past :)" << std::endl;
	std::cout << "Maxim Hoxha 2023" << std::endl << std::endl;
	BroadcastSystems iSys;
	//Note: strcmp returns 0 (=false) if the strings match, so we NOT it to get 1 if the strings match (=true)
	if (argc < 2 || !strcmp(argv[1], "-h"))
	{
		ShowHelp();
		return 0;
	}
	if (!strcmp(argv[1], "-bsyshelp"))
	{
		if (argc < 3)
		{
			std::cout << "Please specify a broadcast system to view info on." << std::endl;
			return 2;
		}
		else if (!strcmp(argv[2], "m")) iSys = BroadcastSystems::M;
		else if (!strcmp(argv[2], "n")) iSys = BroadcastSystems::N;
		else if (!strcmp(argv[2], "b")) iSys = BroadcastSystems::B;
		else if (!strcmp(argv[2], "g")) iSys = BroadcastSystems::G;
		else if (!strcmp(argv[2], "h")) iSys = BroadcastSystems::H;
		else if (!strcmp(argv[2], "i")) iSys = BroadcastSystems::I;
		else if (!strcmp(argv[2], "d")) iSys = BroadcastSystems::D;
		else if (!strcmp(argv[2], "k")) iSys = BroadcastSystems::K;
		else if (!strcmp(argv[2], "l")) iSys = BroadcastSystems::L;
		else if (!strcmp(argv[2], "vhs525")) iSys = BroadcastSystems::VHS525;
		else if (!strcmp(argv[2], "vhs625")) iSys = BroadcastSystems::VHS625;
		DisplayBroadcastSystemInformation(iSys);
		return 0;
	}
	if (argc < 3)
	{
		std::cout << "Please specify a two valid filenames (input then output)!" << std::endl;
		ShowHelp();
		return 1;
	}
	std::cout << "Input video: " << argv[1] << std::endl;
	std::cout << "Output video: " << argv[2] << std::endl;

	BroadcastSystems bSys = BroadcastSystems::I;
	ColourSystems cSys = ColourSystems::PAL;
	bool bSysChosen = false;
	bool ezvhs = false;
	bool preview = false;
	double kbitrate = 10000.0;
	double noise = 0.0;
	double globalPhaseNoise = 0.0;
	double crosstalk = 0.0;
	double phaseNoise = 0.0;
	double jitter = 0.0;
	double dResonance = 5.0;
	double pWidthMult = 0.7;
	for (int i = 3; i < argc; i++)
	{
		if (!strcmp(argv[i], "-csys"))
		{
			i++;
			if (i >= argc) break;
			else if (!strcmp(argv[i], "pal")) cSys = ColourSystems::PAL;
			else if (!strcmp(argv[i], "ntsc")) cSys = ColourSystems::NTSC;
			else if (!strcmp(argv[i], "secam")) cSys = ColourSystems::SECAM;
		}
		else if (!strcmp(argv[i], "-vhs"))
		{
			ezvhs = true;
		}
		else if (!strcmp(argv[i], "-preview"))
		{
			preview = true;
		}
		else if (!strcmp(argv[i], "-bsys"))
		{
			bSysChosen = true;
			i++;
			if (i >= argc) break;
			else if (!strcmp(argv[i], "m")) bSys = BroadcastSystems::M;
			else if (!strcmp(argv[i], "n")) bSys = BroadcastSystems::N;
			else if (!strcmp(argv[i], "b")) bSys = BroadcastSystems::B;
			else if (!strcmp(argv[i], "g")) bSys = BroadcastSystems::G;
			else if (!strcmp(argv[i], "h")) bSys = BroadcastSystems::H;
			else if (!strcmp(argv[i], "i")) bSys = BroadcastSystems::I;
			else if (!strcmp(argv[i], "d")) bSys = BroadcastSystems::D;
			else if (!strcmp(argv[i], "k")) bSys = BroadcastSystems::K;
			else if (!strcmp(argv[i], "l")) bSys = BroadcastSystems::L;
			else if (!strcmp(argv[i], "vhs525")) bSys = BroadcastSystems::VHS525;
			else if (!strcmp(argv[i], "vhs625")) bSys = BroadcastSystems::VHS625;
		}
		else if (!strcmp(argv[i], "-br"))
		{
			i++;
			kbitrate = strtod(argv[i], NULL);
		}
		else if (!strcmp(argv[i], "-noise"))
		{
			i++;
			noise = strtod(argv[i], NULL);
		}
		else if (!strcmp(argv[i], "-jitter"))
		{
			i++;
			jitter = strtod(argv[i], NULL);
		}
		else if (!strcmp(argv[i], "-reso"))
		{
			i++;
			dResonance = strtod(argv[i], NULL);
		}
		else if (!strcmp(argv[i], "-prefreq"))
		{
			i++;
			pWidthMult = strtod(argv[i], NULL);
		}
		else if (!strcmp(argv[i], "-psnoise"))
		{
			i++;
			phaseNoise = strtod(argv[i], NULL);
		}
		else if (!strcmp(argv[i], "-crosstalk"))
		{
			i++;
			crosstalk = strtod(argv[i], NULL);
		}
		else if (!strcmp(argv[i], "-pfnoise"))
		{
			i++;
			globalPhaseNoise = strtod(argv[i], NULL);
		}
		else if (!strcmp(argv[i], "-h"))
		{
			ShowHelp();
			return 0;
		}
		else if (!strcmp(argv[i], "-bsyshelp"))
		{
			i++;
			if (i >= argc) break;
			else if (!strcmp(argv[i], "m")) iSys = BroadcastSystems::M;
			else if (!strcmp(argv[i], "n")) iSys = BroadcastSystems::N;
			else if (!strcmp(argv[i], "b")) iSys = BroadcastSystems::B;
			else if (!strcmp(argv[i], "g")) iSys = BroadcastSystems::G;
			else if (!strcmp(argv[i], "h")) iSys = BroadcastSystems::H;
			else if (!strcmp(argv[i], "i")) iSys = BroadcastSystems::I;
			else if (!strcmp(argv[i], "d")) iSys = BroadcastSystems::D;
			else if (!strcmp(argv[i], "k")) iSys = BroadcastSystems::K;
			else if (!strcmp(argv[i], "l")) iSys = BroadcastSystems::L;
			else if (!strcmp(argv[i], "vhs525")) iSys = BroadcastSystems::VHS525;
			else if (!strcmp(argv[i], "vhs625")) iSys = BroadcastSystems::VHS625;
			DisplayBroadcastSystemInformation(iSys);
			return 0;
		}
	}

	if (!bSysChosen)
	{
		switch (cSys)
		{
		default:
		case ColourSystems::PAL:
			bSys = BroadcastSystems::I;
			break;
		case ColourSystems::NTSC:
			bSys = BroadcastSystems::M;
			break;
		case ColourSystems::SECAM:
			bSys = BroadcastSystems::L;
			break;
		}
	}

	if (ezvhs)
	{
		switch (cSys)
		{
		default:
		case ColourSystems::PAL:
		case ColourSystems::SECAM:
			bSys = BroadcastSystems::VHS625;
			break;
		case ColourSystems::NTSC:
			bSys = BroadcastSystems::VHS525;
			break;
		}
	}

	const char* bSysStr = GetBroadcastSystemDescriptorString(bSys);
	const char* cSysStr = GetColourSystemDescriptorString(cSys);

	std::cout << "Encoding to: " << bSysStr << " " << cSysStr << std::endl;
	std::cout << "Initialising engine..." << std::endl;
	ConversionEngine convEng = ConversionEngine(bSys, cSys, dResonance, pWidthMult);
	convEng.OpenForDecodeVideo(argv[1]);
	std::cout << "Begin encoding." << std::endl;
	convEng.EncodeVideo(argv[2], preview, kbitrate, noise, globalPhaseNoise, crosstalk, phaseNoise, jitter);
	convEng.CloseDecoder();
	std::cout << "Finished conversion!" << std::endl;

	return 0;
}