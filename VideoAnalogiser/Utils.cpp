/*
* VideoAnalogiser - Command Line Utility for Analogising Digital Videos
* Maxim Hoxha 2023
* Mathematical utitlies, mostly relating to filters
* This software uses code of FFmpeg (http://ffmpeg.org) licensed under the LGPLv2.1 (http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html)
*/

#define _USE_MATH_DEFINES
#include <math.h>
#include <cstring>
#include "Utils.h"

#define FILTER_MAKE_INTEGRAL_POINTS 16384
#define FILTER_MAKE_INTEGRAL_POINTS_DBL 16384.0
#define FILTER_MAGNITUDE_TOLERANCE 0.03
#define FILTER_MAX_STEPS_TOLERANCE 7

static inline double StandardFilter(double f, double attenuation)
{
    return 1 / sqrt(1 + pow(fabs(f), 2 * attenuation)); //Butterworth filter, but modified to allow a real (rather than strictly natural) number of 'poles'
}

FIRFilter MakeFIRFilter(double sampleRate, int size, double center, double width, double attenuation)
{
    int backport = 5;
    float* outfir = new float[size + backport];
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

    float* realOutFir = new float[truesize];
    float* fromptr = &outfir[backport - truebackport];
    float* toptr = realOutFir + truesize - 1;
    //Reorder the filter here to aid in vectorising the inner loops of ApplyFIRFilter()
    for (int i = 0; i < truesize; i++)
    {
        *toptr-- = *fromptr++;
    }
    delete[] outfir;

    //Returning this pointer as the zero point to simplify addressing the filter components (i.e. filter[-2] is valid and points to the filter component 2 samples behind the current point)
    return { realOutFir + truesize - truebackport - 1, truesize - truebackport,  truebackport };
}

SignalPack ApplyFIRFilter(SignalPack signal, FIRFilter fir)
{
    float* output = new float[signal.len];
    float outsig = 0.0f;

    for (int i = 0; i < fir.len; i++) //Ease in
    {
        outsig = 0.0f;
        const float* insig = signal.signal + i;
        #pragma omp simd
        for (int j = -i; j <= fir.backport; j++)
        {
            outsig += insig[j] * fir.filter[j];
        }
        output[i] = outsig;
    }

    const int parStart = fir.len;
    const int parEnd = signal.len - fir.backport;
    const int filtStart = -fir.len + 1;
    const int filtEnd = fir.backport;
    const float* const sig = signal.signal;
    const float* const filt = fir.filter;

    //Main loop. This is embarrasingly parallel
    #pragma omp parallel for
    for (int i = parStart; i < parEnd; i++)
    {
        float outsigin = 0.0f;
        const float* insig = sig + i;
        #pragma omp simd
        for (int j = filtStart; j <= filtEnd; j++)
        {
            outsigin += insig[j] * filt[j];
        }
        output[i] = outsigin;
    }

    for (int i = signal.len - fir.backport; i < signal.len; i++) //Ease out
    {
        outsig = 0.0f;
        const float* insig = signal.signal + i;
        #pragma omp simd
        for (int j = -fir.len; j < signal.len - i; j++)
        {
            outsig += insig[j] * fir.filter[j];
        }
        output[i] = outsig;
    }

    return  { output, signal.len };
}

SignalPack ApplyFIRFilterNotch(SignalPack signal, FIRFilter fir)
{
    float* shiftfir = new float[fir.len + fir.backport];
    float* actualShiftfir = shiftfir + fir.len - 1;

    for (int i = -fir.len + 1; i <= fir.backport; i++)
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
    float* shiftfir = new float[fir.len + fir.backport];
    float* actualShiftfir = shiftfir + fir.len - 1;

    for (int i = -fir.len + 1; i <= fir.backport; i++)
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
    float* shiftfir = new float[fir.len + fir.backport];
    float* actualShiftfir = shiftfir + fir.len - 1;
    double time = 0.0;

    for (int i = -fir.len + 1; i <= fir.backport; i++)
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
    float* shiftfir = new float[fir.len + fir.backport];
    float* actualShiftfir = shiftfir + fir.len - 1;

    for (int i = -fir.len + 1; i <= fir.backport; i++)
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
    float* shiftfir = new float[fir.len + fir.backport];
    float* actualShiftfir = shiftfir + fir.len - 1;
    double time = 0.0;

    for (int i = -fir.len + 1; i <= fir.backport; i++)
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
    float* shiftfir = new float[fir.len + fir.backport];
    float* actualShiftfir = shiftfir + fir.len - 1;
    double time = 0.0;

    for (int i = -fir.len + 1; i <= fir.backport; i++)
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
    float* shiftfir = new float[fir.len + fir.backport];
    float* actualShiftfir = shiftfir + fir.len - 1;
    double time = 0.0;

    for (int i = -fir.len + 1; i <= fir.backport; i++)
    {
        time = i * sampleTime;
        actualShiftfir[i] = fir.filter[i] * cos(centerangfreq * time) * (crosstalk - 1.0) * 2.0;
    }
    actualShiftfir[0] = 1.0 + (crosstalk - 1.0) * fir.filter[0];

    SignalPack outsig = ApplyFIRFilter(signal, { actualShiftfir, fir.len, fir.backport });
    delete[] shiftfir;
    return outsig;
}