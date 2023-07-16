/*
* VideoAnalogiser - Command Line Utility for Analogising Digital Videos
* Maxim Hoxha 2023
* Mathematical utitlies, mostly relating to filters
* This software uses code of FFmpeg (http://ffmpeg.org) licensed under the LGPLv2.1 (http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html)
*/

#define _USE_MATH_DEFINES
#include <math.h>
#include <memory>
#include "Utils.h"
#include <omp.h>

#define FILTER_MAKE_INTEGRAL_POINTS 16384
#define FILTER_MAKE_INTEGRAL_POINTS_DBL 16384.0
#define FILTER_MAGNITUDE_TOLERANCE 0.001
#define FILTER_MAX_STEPS_TOLERANCE 6

static inline double StandardFilter(double f, double attenuation)
{
    return 1 / sqrt(1 + pow(fabs(f), 2 * attenuation)); //Butterworth filter, but modified to allow a real (rather than strictly natural) number of 'poles'
}

FIRFilter MakeFIRFilter(double sampleRate, int size, double center, double width, double attenuation)
{
    int backport = 5;
    double* outfir = new double[size + backport];
    double integral = 0.0;
    double integpoint = 0.0;
    double freqpointbef = 0.0;
    double freqpointmid = 0.0;
    double freqpointaf = 0.0;
    double sampleTime = 1 / sampleRate;
    double trueW = 1 / (width * 0.5);
    int truesize = 0;
    int truebackport = 0;
    int stepsUnderTolerance = 0;

    for (int i = 1; i <= backport; i++) //Do filter components from AFTER the zero point first
    {
        integral = 0.0;
        for (int j = 0; j < FILTER_MAKE_INTEGRAL_POINTS; j++) //Integrate with Simpson's rule
        {
            //The following integral bounds may seem strange, but they were found to create better filters that don't require rescaling
            freqpointbef = (sampleRate * ((((double)j) / FILTER_MAKE_INTEGRAL_POINTS_DBL) - 0.5)) + center;
            freqpointaf = (sampleRate * ((((double)(j + 1)) / FILTER_MAKE_INTEGRAL_POINTS_DBL) - 0.5)) + center;
            freqpointmid = (freqpointaf + freqpointbef) * 0.5;
            integpoint = cos(-2.0 * M_PI * freqpointbef * sampleTime * i) * StandardFilter((freqpointbef - center) * trueW, attenuation);
            integpoint += (4.0 * cos(-2.0 * M_PI * freqpointmid * sampleTime * i)) * StandardFilter((freqpointmid - center) * trueW, attenuation);
            integpoint += cos(-2.0 * M_PI * freqpointaf * sampleTime * i) * StandardFilter((freqpointaf - center) * trueW, attenuation);
            integpoint *= (freqpointaf - freqpointbef) / 6.0;
            integral += integpoint / sampleRate;
        }
        outfir[backport - i] = integral;
        truesize++;
        truebackport++;
        if (fabs(integral) < FILTER_MAGNITUDE_TOLERANCE)
        {
            stepsUnderTolerance++;
        }
        else
        {
            stepsUnderTolerance = 0;
        }
        if (stepsUnderTolerance >= FILTER_MAX_STEPS_TOLERANCE)
        {
            break;
        }
    }
    stepsUnderTolerance = 0;
    for (int i = 0; i < size; i++) //Then do it for the filter components ON and BEFORE the zero point
    {
        integral = 0.0;
        for (int j = 0; j < FILTER_MAKE_INTEGRAL_POINTS; j++) //Integrate with Simpson's rule
        {
            //The following integral bounds may seem strange, but they were found to create better filters that don't require rescaling
            freqpointbef = (sampleRate * ((((double)j) / FILTER_MAKE_INTEGRAL_POINTS_DBL) - 0.5)) + center;
            freqpointaf = (sampleRate * ((((double)(j + 1)) / FILTER_MAKE_INTEGRAL_POINTS_DBL) - 0.5)) + center;
            freqpointmid = (freqpointaf + freqpointbef) * 0.5;
            integpoint = cos(2.0 * M_PI * freqpointbef * sampleTime * i) * StandardFilter((freqpointbef - center) * trueW, attenuation);
            integpoint += (4.0 * cos(2.0 * M_PI * freqpointmid * sampleTime * i)) * StandardFilter((freqpointmid - center) * trueW, attenuation);
            integpoint += cos(2.0 * M_PI * freqpointaf * sampleTime * i) * StandardFilter((freqpointaf - center) * trueW, attenuation);
            integpoint *= (freqpointaf - freqpointbef) / 6.0;
            integral += integpoint / sampleRate;
        }
        if (abs(i) > truebackport) integral *= 2.0;
        outfir[i + backport] = integral;
        truesize++;
        if (fabs(integral) < FILTER_MAGNITUDE_TOLERANCE)
        {
            stepsUnderTolerance++;
        }
        else
        {
            stepsUnderTolerance = 0;
        }
        if (stepsUnderTolerance >= FILTER_MAX_STEPS_TOLERANCE)
        {
            break;
        }
    }

    double* realOutFir = new double[truesize];
    memcpy(realOutFir, &outfir[backport - truebackport], truesize * 8);
    delete[] outfir;

    //Returning this pointer as the zero point to simplify addressing the filter components (i.e. filter[-2] is valid and points to the filter component 2 samples ahead of the current point)
    return { realOutFir + truebackport, truesize - truebackport,  truebackport };
}

FIRFilter MakeNotch(FIRFilter filt) //This function has become redundant as it was found to be suboptimal for the overall process
{
    double* outdat = new double[filt.len + filt.backport];
    double* realoutdat = outdat + filt.backport;
    FIRFilter outfir = { realoutdat, filt.len, filt.backport };
    realoutdat[0] = 1.0 - filt.filter[0];
    for (int i = 1; i < outfir.len; i++)
    {
        realoutdat[i] = -filt.filter[i];
    }
    for (int i = 1; i <= outfir.backport; i++)
    {
        realoutdat[-i] = -filt.filter[-i];
    }

    return outfir;
}

SignalPack ApplyFIRFilter(SignalPack signal, FIRFilter fir)
{
    double* output = new double[signal.len];
    double outsig = 0.0;

    for (int i = 0; i < fir.len; i++) //Ease in
    {
        outsig = 0.0;
        for (int j = -fir.backport; j <= i; j++)
        {
            outsig += signal.signal[i - j] * fir.filter[j];
        }
        output[i] = outsig;
    }

    const int parStart = fir.len;
    const int parEnd = signal.len - fir.backport;
    const int filtStart = -fir.backport;
    const int filtEnd = fir.len;
    const double* const sig = signal.signal;
    const double* const filt = fir.filter;

    //Main loop. This is embarrasingly parallel
    omp_set_num_threads(omp_get_max_threads());
    #pragma omp parallel for shared(sig, filt, output, filtStart, filtEnd)
    for (int i = parStart; i < parEnd; i++)
    {
        double outsigin = 0.0;
        for (int j = filtStart; j < filtEnd; j++)
        {
            outsigin += sig[i - j] * filt[j];
        }
        output[i] = outsigin;
    }

    for (int i = signal.len - fir.backport; i < signal.len; i++) //Ease out
    {
        outsig = 0.0;
        for (int j = i - signal.len + 1; j <= fir.len; j++)
        {
            outsig += signal.signal[i - j] * fir.filter[j];
        }
        output[i] = outsig;
    }

    return  { output, signal.len };
}

SignalPack ApplyFIRFilterNotch(SignalPack signal, FIRFilter fir)
{
    double* shiftfir = new double[fir.len + fir.backport];
    double* actualShiftfir = shiftfir + fir.backport;

    for (int i = -fir.backport; i < fir.len; i++)
    {
        actualShiftfir[i] = -fir.filter[i];
    }
    actualShiftfir[0] = 1.0 - fir.filter[0];

    SignalPack outsig = ApplyFIRFilter(signal, { actualShiftfir, fir.len, fir.backport });
    delete[] shiftfir;
    return outsig;
}

SignalPack ApplyFIRFilterCrosstalk(SignalPack signal, FIRFilter fir, double crosstalk)
{
    double* shiftfir = new double[fir.len + fir.backport];
    double* actualShiftfir = shiftfir + fir.backport;

    for (int i = -fir.backport; i < fir.len; i++)
    {
        actualShiftfir[i] = fir.filter[i] * (1.0 - crosstalk);
    }
    actualShiftfir[0] = (1.0 - crosstalk) * fir.filter[0] + crosstalk;

    SignalPack outsig = ApplyFIRFilter(signal, { actualShiftfir, fir.len, fir.backport });
    delete[] shiftfir;
    return outsig;
}

SignalPack ApplyFIRFilterShift(SignalPack signal, FIRFilter fir, double sampleTime, double centerangfreq)
{
    double* shiftfir = new double[fir.len + fir.backport];
    double* actualShiftfir = shiftfir + fir.backport;
    double time = 0.0;

    for (int i = -fir.backport; i < fir.len; i++)
    {
        time = i * sampleTime;
        actualShiftfir[i] = fir.filter[i] * cos(centerangfreq * time) * 2.0; //This takes advantage of a crucial property of Fourier transforms
    }

    SignalPack outsig = ApplyFIRFilter(signal, { actualShiftfir, fir.len, fir.backport });
    delete[] shiftfir;
    return outsig;
}

SignalPack ApplyFIRFilterNotchCrosstalk(SignalPack signal, FIRFilter fir, double crosstalk)
{
    double* shiftfir = new double[fir.len + fir.backport];
    double* actualShiftfir = shiftfir + fir.backport;

    for (int i = -fir.backport; i < fir.len; i++)
    {
        actualShiftfir[i] = fir.filter[i] * (crosstalk - 1.0);
    }
    actualShiftfir[0] = 1.0 + (crosstalk - 1.0) * fir.filter[0];

    SignalPack outsig = ApplyFIRFilter(signal, { actualShiftfir, fir.len, fir.backport });
    delete[] shiftfir;
    return outsig;
}

SignalPack ApplyFIRFilterCrosstalkShift(SignalPack signal, FIRFilter fir, double crosstalk, double sampleTime, double centerangfreq)
{
    double* shiftfir = new double[fir.len + fir.backport];
    double* actualShiftfir = shiftfir + fir.backport;
    double time = 0.0;

    for (int i = -fir.backport; i < fir.len; i++)
    {
        time = i * sampleTime;
        actualShiftfir[i] = fir.filter[i] * cos(centerangfreq * time) * (1.0 - crosstalk) * 2.0;
    }
    actualShiftfir[0] = (1.0 - crosstalk) * fir.filter[0] + crosstalk;

    SignalPack outsig = ApplyFIRFilter(signal, { actualShiftfir, fir.len, fir.backport });
    delete[] shiftfir;
    return outsig;
}

SignalPack ApplyFIRFilterNotchShift(SignalPack signal, FIRFilter fir, double sampleTime, double centerangfreq)
{
    double* shiftfir = new double[fir.len + fir.backport];
    double* actualShiftfir = shiftfir + fir.backport;
    double time = 0.0;

    for (int i = -fir.backport; i < fir.len; i++)
    {
        time = i * sampleTime;
        actualShiftfir[i] = -fir.filter[i] * cos(centerangfreq * time) * 2.0;
    }
    actualShiftfir[0] = 1.0 - fir.filter[0];

    SignalPack outsig = ApplyFIRFilter(signal, { actualShiftfir, fir.len, fir.backport });
    delete[] shiftfir;
    return outsig;
}

SignalPack ApplyFIRFilterNotchCrosstalkShift(SignalPack signal, FIRFilter fir, double crosstalk, double sampleTime, double centerangfreq)
{
    double* shiftfir = new double[fir.len + fir.backport];
    double* actualShiftfir = shiftfir + fir.backport;
    double time = 0.0;

    for (int i = -fir.backport; i < fir.len; i++)
    {
        time = i * sampleTime;
        actualShiftfir[i] = fir.filter[i] * cos(centerangfreq * time) * (crosstalk - 1.0) * 2.0;
    }
    actualShiftfir[0] = 1.0 + (crosstalk - 1.0) * fir.filter[0];

    SignalPack outsig = ApplyFIRFilter(signal, { actualShiftfir, fir.len, fir.backport });
    delete[] shiftfir;
    return outsig;
}