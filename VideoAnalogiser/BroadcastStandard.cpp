/*
* VideoAnalogiser - Command Line Utility for Analogising Digital Videos
* Maxim Hoxha 2023
* Broadcast standard definitions
* This software uses code of FFmpeg (http://ffmpeg.org) licensed under the LGPLv2.1 (http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html)
*/

#define _USE_MATH_DEFINES
#include <math.h>
#include <iostream>
#include "BroadcastStandard.h"

//Note: not all combinations of colour system and broadcast standard were actually used, so SECAM-specific values are essentially faked for any system that never used SECAM

const BroadcastStandard SystemM = { 4.2e+6,  //Main Bandwidth
                              0.75e+6, //Side Bandwidth
                              1.3e+6, //Colour bandwidth lower part
                              0.62e+6, //Colour bandwidth upper part
                              3579545.0, //Colour subcarrier frequency
                              525, //Total scanlines
                              480, //Visible scanlines
                              59.94005994, //Nominal framerate
                              {1001, 60000}, //Nominal timebase (for libav*)
                              5.26555e-5, //Nominal active video time (per scanline)
                              3579545.0 * 2.0 * M_PI, //Colour subcarrier angular frequency
                              1.0 / 59.94005994, //Nominal frametime
                              1.0 / (59.94005994 * 525.0), //Nominal scanline time
                              3501420.0, //Db subcarrier frequency (SECAM only)
                              3657670.0, //Dr subcarrier frequency (SECAM only)
                              1012000.0, //Db bandwidth lower part (SECAM only)
                              700000.0, //Db bandwidth upper part (SECAM only)
                              700000.0, //Dr bandwidth lower part (SECAM only)
                              1012000.0, //Dr bandwidth upper part (SECAM only)
                              230000.0 * 2.0 * M_PI, //Db frequency shift amount (SECAM only)
                              280000.0 * 2.0 * M_PI, //Dr frequency shift amount (SECAM only)
                              3501420.0 * 2.0 * M_PI, //Db subcarrier angular frequency (SECAM only)
                              3657670.0 * 2.0 * M_PI, //Dr subcarrier angular frequency (SECAM only)
};

const BroadcastStandard SystemN = { 4.2e+6,  //Main Bandwidth
                              0.75e+6, //Side Bandwidth
                              1.3e+6, //Colour bandwidth lower part
                              0.57e+6, //Colour bandwidth upper part
                              4433618.75, //Colour subcarrier frequency
                              625, //Total scanlines
                              576, //Visible scanlines
                              50.0, //Nominal framerate
                              {1, 50}, //Nominal timebase (for libav*)
                              5.2e-5, //Nominal active video time (per scanline)
                              4433618.75 * 2.0 * M_PI, //Colour subcarrier angular frequency
                              1.0 / 50.0, //Nominal frametime
                              1.0 / (50.0 * 625.0), //Nominal scanline time
                              4250000.0, //Db subcarrier frequency (SECAM only)
                              4406250.0, //Dr subcarrier frequency (SECAM only)
                              1012000.0, //Db bandwidth lower part (SECAM only)
                              700000.0, //Db bandwidth upper part (SECAM only)
                              700000.0, //Dr bandwidth lower part (SECAM only)
                              1012000.0, //Dr bandwidth upper part (SECAM only)
                              230000.0 * 2.0 * M_PI, //Db frequency shift amount (SECAM only)
                              280000.0 * 2.0 * M_PI, //Dr frequency shift amount (SECAM only)
                              4250000.0 * 2.0 * M_PI, //Db subcarrier angular frequency (SECAM only)
                              4406250.0 * 2.0 * M_PI, //Dr subcarrier angular frequency (SECAM only)
};

const BroadcastStandard SystemB = { 5.0e+6,  //Main Bandwidth
                              0.75e+6, //Side Bandwidth
                              1.3e+6, //Colour bandwidth lower part
                              0.57e+6, //Colour bandwidth upper part
                              4433618.75, //Colour subcarrier frequency
                              625, //Total scanlines
                              576, //Visible scanlines
                              50.0, //Nominal framerate
                              {1, 50}, //Nominal timebase (for libav*)
                              5.2e-5, //Nominal active video time (per scanline)
                              4433618.75 * 2.0 * M_PI, //Colour subcarrier angular frequency
                              1.0 / 50.0, //Nominal frametime
                              1.0 / (50.0 * 625.0), //Nominal scanline time
                              4250000.0, //Db subcarrier frequency (SECAM only)
                              4406250.0, //Dr subcarrier frequency (SECAM only)
                              1012000.0, //Db bandwidth lower part (SECAM only)
                              700000.0, //Db bandwidth upper part (SECAM only)
                              700000.0, //Dr bandwidth lower part (SECAM only)
                              1012000.0, //Dr bandwidth upper part (SECAM only)
                              230000.0 * 2.0 * M_PI, //Db frequency shift amount (SECAM only)
                              280000.0 * 2.0 * M_PI, //Dr frequency shift amount (SECAM only)
                              4250000.0 * 2.0 * M_PI, //Db subcarrier angular frequency (SECAM only)
                              4406250.0 * 2.0 * M_PI, //Dr subcarrier angular frequency (SECAM only)
};

const BroadcastStandard SystemG = { 5.0e+6,  //Main Bandwidth
                              0.75e+6, //Side Bandwidth
                              1.3e+6, //Colour bandwidth lower part
                              0.57e+6, //Colour bandwidth upper part
                              4433618.75, //Colour subcarrier frequency
                              625, //Total scanlines
                              576, //Visible scanlines
                              50.0, //Nominal framerate
                              {1, 50}, //Nominal timebase (for libav*)
                              5.2e-5, //Nominal active video time (per scanline)
                              4433618.75 * 2.0 * M_PI, //Colour subcarrier angular frequency
                              1.0 / 50.0, //Nominal frametime
                              1.0 / (50.0 * 625.0), //Nominal scanline time
                              4250000.0, //Db subcarrier frequency (SECAM only)
                              4406250.0, //Dr subcarrier frequency (SECAM only)
                              1012000.0, //Db bandwidth lower part (SECAM only)
                              700000.0, //Db bandwidth upper part (SECAM only)
                              700000.0, //Dr bandwidth lower part (SECAM only)
                              1012000.0, //Dr bandwidth upper part (SECAM only)
                              230000.0 * 2.0 * M_PI, //Db frequency shift amount (SECAM only)
                              280000.0 * 2.0 * M_PI, //Dr frequency shift amount (SECAM only)
                              4250000.0 * 2.0 * M_PI, //Db subcarrier angular frequency (SECAM only)
                              4406250.0 * 2.0 * M_PI, //Dr subcarrier angular frequency (SECAM only)
};

const BroadcastStandard SystemH = { 5.0e+6,  //Main Bandwidth
                              1.25e+6, //Side Bandwidth
                              1.3e+6, //Colour bandwidth lower part
                              0.57e+6, //Colour bandwidth upper part
                              4433618.75, //Colour subcarrier frequency
                              625, //Total scanlines
                              576, //Visible scanlines
                              50.0, //Nominal framerate
                              {1, 50}, //Nominal timebase (for libav*)
                              5.2e-5, //Nominal active video time (per scanline)
                              4433618.75 * 2.0 * M_PI, //Colour subcarrier angular frequency
                              1.0 / 50.0, //Nominal frametime
                              1.0 / (50.0 * 625.0), //Nominal scanline time
                              4250000.0, //Db subcarrier frequency (SECAM only)
                              4406250.0, //Dr subcarrier frequency (SECAM only)
                              1012000.0, //Db bandwidth lower part (SECAM only)
                              700000.0, //Db bandwidth upper part (SECAM only)
                              700000.0, //Dr bandwidth lower part (SECAM only)
                              1012000.0, //Dr bandwidth upper part (SECAM only)
                              230000.0 * 2.0 * M_PI, //Db frequency shift amount (SECAM only)
                              280000.0 * 2.0 * M_PI, //Dr frequency shift amount (SECAM only)
                              4250000.0 * 2.0 * M_PI, //Db subcarrier angular frequency (SECAM only)
                              4406250.0 * 2.0 * M_PI, //Dr subcarrier angular frequency (SECAM only)
};

const BroadcastStandard SystemI = { 5.5e+6,  //Main Bandwidth
                              1.25e+6, //Side Bandwidth
                              1.3e+6, //Colour bandwidth lower part
                              1.066e+6, //Colour bandwidth upper part
                              4433618.75, //Colour subcarrier frequency
                              625, //Total scanlines
                              576, //Visible scanlines
                              50.0, //Nominal framerate
                              {1, 50}, //Nominal timebase (for libav*)
                              5.2e-5, //Nominal active video time (per scanline)
                              4433618.75 * 2.0 * M_PI, //Colour subcarrier angular frequency
                              1.0 / 50.0, //Nominal frametime
                              1.0 / (50.0 * 625.0), //Nominal scanline time
                              4250000.0, //Db subcarrier frequency (SECAM only)
                              4406250.0, //Dr subcarrier frequency (SECAM only)
                              1012000.0, //Db bandwidth lower part (SECAM only)
                              700000.0, //Db bandwidth upper part (SECAM only)
                              700000.0, //Dr bandwidth lower part (SECAM only)
                              1012000.0, //Dr bandwidth upper part (SECAM only)
                              230000.0 * 2.0 * M_PI, //Db frequency shift amount (SECAM only)
                              280000.0 * 2.0 * M_PI, //Dr frequency shift amount (SECAM only)
                              4250000.0 * 2.0 * M_PI, //Db subcarrier angular frequency (SECAM only)
                              4406250.0 * 2.0 * M_PI, //Dr subcarrier angular frequency (SECAM only)
};

const BroadcastStandard SystemD = { 6.0e+6,  //Main Bandwidth
                              0.75e+6, //Side Bandwidth
                              1.3e+6, //Colour bandwidth lower part
                              0.57e+6, //Colour bandwidth upper part
                              4433618.75, //Colour subcarrier frequency
                              625, //Total scanlines
                              576, //Visible scanlines
                              50.0, //Nominal framerate
                              {1, 50}, //Nominal timebase (for libav*)
                              5.2e-5, //Nominal active video time (per scanline)
                              4433618.75 * 2.0 * M_PI, //Colour subcarrier angular frequency
                              1.0 / 50.0, //Nominal frametime
                              1.0 / (50.0 * 625.0), //Nominal scanline time
                              4250000.0, //Db subcarrier frequency (SECAM only)
                              4406250.0, //Dr subcarrier frequency (SECAM only)
                              1012000.0, //Db bandwidth lower part (SECAM only)
                              700000.0, //Db bandwidth upper part (SECAM only)
                              700000.0, //Dr bandwidth lower part (SECAM only)
                              1012000.0, //Dr bandwidth upper part (SECAM only)
                              230000.0 * 2.0 * M_PI, //Db frequency shift amount (SECAM only)
                              280000.0 * 2.0 * M_PI, //Dr frequency shift amount (SECAM only)
                              4250000.0 * 2.0 * M_PI, //Db subcarrier angular frequency (SECAM only)
                              4406250.0 * 2.0 * M_PI, //Dr subcarrier angular frequency (SECAM only)
};

const BroadcastStandard SystemK = { 6.0e+6,  //Main Bandwidth
                              0.75e+6, //Side Bandwidth
                              1.3e+6, //Colour bandwidth lower part
                              0.57e+6, //Colour bandwidth upper part
                              4433618.75, //Colour subcarrier frequency
                              625, //Total scanlines
                              576, //Visible scanlines
                              50.0, //Nominal framerate
                              {1, 50}, //Nominal timebase (for libav*)
                              5.2e-5, //Nominal active video time (per scanline)
                              4433618.75 * 2.0 * M_PI, //Colour subcarrier angular frequency
                              1.0 / 50.0, //Nominal frametime
                              1.0 / (50.0 * 625.0), //Nominal scanline time
                              4250000.0, //Db subcarrier frequency (SECAM only)
                              4406250.0, //Dr subcarrier frequency (SECAM only)
                              1012000.0, //Db bandwidth lower part (SECAM only)
                              700000.0, //Db bandwidth upper part (SECAM only)
                              700000.0, //Dr bandwidth lower part (SECAM only)
                              1012000.0, //Dr bandwidth upper part (SECAM only)
                              230000.0 * 2.0 * M_PI, //Db frequency shift amount (SECAM only)
                              280000.0 * 2.0 * M_PI, //Dr frequency shift amount (SECAM only)
                              4250000.0 * 2.0 * M_PI, //Db subcarrier angular frequency (SECAM only)
                              4406250.0 * 2.0 * M_PI, //Dr subcarrier angular frequency (SECAM only)
};

const BroadcastStandard SystemL = { 6.0e+6,  //Main Bandwidth
                              1.25e+6, //Side Bandwidth
                              1.3e+6, //Colour bandwidth lower part
                              1.066e+6, //Colour bandwidth upper part
                              4433618.75, //Colour subcarrier frequency
                              625, //Total scanlines
                              576, //Visible scanlines
                              50.0, //Nominal framerate
                              {1, 50}, //Nominal timebase (for libav*)
                              5.2e-5, //Nominal active video time (per scanline)
                              4433618.75 * 2.0 * M_PI, //Colour subcarrier angular frequency
                              1.0 / 50.0, //Nominal frametime
                              1.0 / (50.0 * 625.0), //Nominal scanline time
                              4250000.0, //Db subcarrier frequency (SECAM only)
                              4406250.0, //Dr subcarrier frequency (SECAM only)
                              1012000.0, //Db bandwidth lower part (SECAM only)
                              700000.0, //Db bandwidth upper part (SECAM only)
                              700000.0, //Dr bandwidth lower part (SECAM only)
                              1012000.0, //Dr bandwidth upper part (SECAM only)
                              230000.0 * 2.0 * M_PI, //Db frequency shift amount (SECAM only)
                              280000.0 * 2.0 * M_PI, //Dr frequency shift amount (SECAM only)
                              4250000.0 * 2.0 * M_PI, //Db subcarrier angular frequency (SECAM only)
                              4406250.0 * 2.0 * M_PI, //Dr subcarrier angular frequency (SECAM only)
};

const BroadcastStandard TapeVHS525 =  { 3.4e+6,  //Main Bandwidth
                              0.1e+6, //Side Bandwidth
                              0.629e+6, //Colour bandwidth lower part
                              0.629e+6, //Colour bandwidth upper part
                              3579545.0, //Colour subcarrier frequency
                              525, //Total scanlines
                              480, //Visible scanlines
                              59.94005994, //Nominal framerate
                              {1001, 60000}, //Nominal timebase (for libav*)
                              5.26555e-5, //Nominal active video time (per scanline)
                              3579545.0 * 2.0 * M_PI, //Colour subcarrier angular frequency
                              1.0 / 59.94005994, //Nominal frametime
                              1.0 / (59.94005994 * 525.0), //Nominal scanline time
                              3501420.0, //Db subcarrier frequency (SECAM only)
                              3657670.0, //Dr subcarrier frequency (SECAM only)
                              1012000.0, //Db bandwidth lower part (SECAM only)
                              700000.0, //Db bandwidth upper part (SECAM only)
                              700000.0, //Dr bandwidth lower part (SECAM only)
                              1012000.0, //Dr bandwidth upper part (SECAM only)
                              230000.0 * 2.0 * M_PI, //Db frequency shift amount (SECAM only)
                              280000.0 * 2.0 * M_PI, //Dr frequency shift amount (SECAM only)
                              3501420.0 * 2.0 * M_PI, //Db subcarrier angular frequency (SECAM only)
                              3657670.0 * 2.0 * M_PI, //Dr subcarrier angular frequency (SECAM only)
};

const BroadcastStandard TapeVHS625 = { 3.4e+6,  //Main Bandwidth
                              0.1e+6, //Side Bandwidth
                              0.629e+6, //Colour bandwidth lower part
                              0.629e+6, //Colour bandwidth upper part
                              4433618.75, //Colour subcarrier frequency
                              625, //Total scanlines
                              576, //Visible scanlines
                              50.0, //Nominal framerate
                              {1, 50}, //Nominal timebase (for libav*)
                              5.2e-5, //Nominal active video time (per scanline)
                              4433618.75 * 2.0 * M_PI, //Colour subcarrier angular frequency
                              1.0 / 50.0, //Nominal frametime
                              1.0 / (50.0 * 625.0), //Nominal scanline time
                              4250000.0, //Db subcarrier frequency (SECAM only)
                              4406250.0, //Dr subcarrier frequency (SECAM only)
                              1012000.0, //Db bandwidth lower part (SECAM only)
                              700000.0, //Db bandwidth upper part (SECAM only)
                              700000.0, //Dr bandwidth lower part (SECAM only)
                              1012000.0, //Dr bandwidth upper part (SECAM only)
                              230000.0 * 2.0 * M_PI, //Db frequency shift amount (SECAM only)
                              280000.0 * 2.0 * M_PI, //Dr frequency shift amount (SECAM only)
                              4250000.0 * 2.0 * M_PI, //Db subcarrier angular frequency (SECAM only)
                              4406250.0 * 2.0 * M_PI, //Dr subcarrier angular frequency (SECAM only)
};

const char* GetBroadcastSystemDescriptorString(BroadcastSystems bSys)
{
    switch (bSys)
    {
    case BroadcastSystems::M:
        return "CCIR System M";
    case BroadcastSystems::N:
        return "CCIR System N";
    case BroadcastSystems::B:
        return "CCIR System B";
    case BroadcastSystems::G:
        return "CCIR System G";
    case BroadcastSystems::H:
        return "CCIR System H";
    default: //just because i live in the UK
    case BroadcastSystems::I:
        return "CCIR System I";
    case BroadcastSystems::D:
        return "CCIR System D";
    case BroadcastSystems::K:
        return "CCIR System K";
    case BroadcastSystems::L:
        return "CCIR System L";
    case BroadcastSystems::VHS525:
        return "VHS (525 line standard)";
    case BroadcastSystems::VHS625:
        return "VHS (625 line standard)";
    }
}

void DisplayBroadcastSystemInformation(BroadcastSystems bSys)
{
    const BroadcastStandard* bcParams;
    switch (bSys)
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
    printf("Name: %s\n", GetBroadcastSystemDescriptorString(bSys));
    printf("Main bandwidth: %.4f MHz\n", bcParams->mainBandwidth / 1e+6);
    printf("Side bandwidth: %.4f MHz\n", bcParams->sideBandwidth / 1e+6);
    printf("Colour bandwidth lower part: %.4f MHz\n", bcParams->chromaBandwidthLower / 1e+6);
    printf("Colour bandwidth upper part: %.4f MHz\n", bcParams->chromaBandwidthUpper / 1e+6);
    printf("Colour subcarrier frequency: %.4f MHz\n", bcParams->chromaCarrierFrequency / 1e+6);
    printf("Total scanlines: %d\n", bcParams->scanlines);
    printf("Visible scanlines: %d\n", bcParams->videoScanlines);
    printf("Field rate: %.4f Hz\n", bcParams->framerate);
    printf("Active video time (per scanline): %.4f us\n", bcParams->activeTime / 1e-6);
}