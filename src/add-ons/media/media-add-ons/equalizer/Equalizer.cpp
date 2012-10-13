/*
 * Copyright 2012, Gerasim Troeglazov (3dEyes**), 3dEyes@gmail.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
 
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "Equalizer.h"

Equalizer::Equalizer()
{	
	fActivated = false;
	CleanUp();	
}

Equalizer::~Equalizer()
{	
}

void
Equalizer::SetFormat(int channels, float framerate)
{	
    fChannels = channels;
    fRate = framerate;

    memset(fWDataVector[0][0], 0, sizeof(fWDataVector));

    fActiveBands = BandCount();
    while(fFrequency[fActiveBands-1] * 2.0 > fRate) {
    	fActiveBands--;
    }

    for(int i = 0; i < fActiveBands; i++) {
        BandPassFilterCalcs(fAWeights[i], fBWeights[i],
        					fFrequency[i] / (float)fRate);
    }    
}

void
Equalizer::SetPreAmp(double value)
{	
	fPreAmp = value;
	RecalcGains();
}

double
Equalizer::PreAmp(void)
{	
	return fPreAmp;
}

void
Equalizer::SetBand(int band, double value)
{	
	if(band < 0 || band >= BandCount()) {
		return;
	}
	fBands[band] = value;
	RecalcGains();
}

double
Equalizer::Band(int band)
{
	if(band < 0 || band >= BandCount()) {
		return 0.0;
	}
	return fBands[band];
}

int
Equalizer::BandCount(void)
{
	return EQ_BANDS;
}

float
Equalizer::BandFrequency(int band)
{
	if(band < 0 || band >= BandCount()) {
		return 0.0;
	}	
	return fFrequency[band];
}

void
Equalizer::ProcessBuffer(float *buffer, int samples)
{
   if(!fActivated || samples <= 0) {
        return;
   }

   for(int i = 0; i < fChannels; i++) {
		float *g = fGainVector[i];
      	float *end = buffer + samples;
        for(float *fptr = buffer + i; fptr < end; fptr += fChannels) {
            float yt = *fptr;
            for (int k = 0; k < fActiveBands; k ++) {
                float *Wq = fWDataVector[i][k];
                float w = yt * fBWeights[k][0] + 
                		Wq[0] * fAWeights[k][0] + 
                		Wq[1] * fAWeights[k][1];
                yt += (w + Wq[1] * fBWeights[k][1]) * g[k];
                Wq[1] = Wq[0];
                Wq[0] = w;
            }
            *fptr = yt;
        }
    }
}

void
Equalizer::CleanUp()
{
	float freq[EQ_BANDS] = {31.25, 62.5, 125, 250, 500, 1000, 2000, 4000, 8000, 16000};

	for(int i = 0; i < BandCount(); i++) {
		fFrequency[i] = freq[i];
		fBands[i] = 0.0;
	}
	
	fPreAmp = 0.0;
	RecalcGains();
}

void
Equalizer::RecalcGains(void)
{
    float adjust[EQ_BANDS];
    for(int i = 0; i < BandCount(); i++) {
        adjust[i] = fPreAmp + fBands[i];
    }

    for(int c = 0; c < MAX_CHANNELS; c++) {
    	for(int i = 0; i<BandCount(); i++) {
     	   fGainVector[c][i] = pow(10, adjust[i] / 20) - 1;
    	}
    }
    fActivated = true;
}

void
Equalizer::BandPassFilterCalcs(float *a, float *b, float f)
{
	float q = 1.2247449;
    float theta = 2.0 * M_PI * f;
    float c_value = (1 - tanf(theta * q / 2.0)) / (1 + tanf(theta * q / 2.0));
    
    a[0] = (1 + c_value) * cosf(theta); 
    a[1] = -c_value;
    b[0] = (1 - c_value) / 2.0;
    b[1] = -1.005;
}
