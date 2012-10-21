/*
 * Copyright 2012, Gerasim Troeglazov (3dEyes**), 3dEyes@gmail.com.
 * All rights reserved.
 * Distributed under the terms of the MIT License.
 */
 
#ifndef __EQUALIZER_H__
#define __EQUALIZER_H__

#define MAX_CHANNELS 8
#define EQ_BANDS 10

class Equalizer {
public:
			Equalizer();
			~Equalizer();
	void	SetFormat(int channels, float framerate);
	void	SetPreAmp(double);
	double	PreAmp(void);
	void	SetBand(int band, double value);
	double	Band(int band);
	int		BandCount(void);
	float	BandFrequency(int band);
	void	ProcessBuffer(float *buffer, int samples);
	void	CleanUp();
private:	
	void 	RecalcGains(void);
	void	BandPassFilterCalcs(float *a, float *b, float f);
	
	bool 	fActivated;
	int	 	fChannels;
	float 	fRate;	
	int 	fActiveBands;
	
	float 	fAWeights[EQ_BANDS][2];
	float 	fBWeights[EQ_BANDS][2];
	float 	fWDataVector[MAX_CHANNELS][EQ_BANDS][2];
	float 	fGainVector[MAX_CHANNELS][EQ_BANDS];	
	float 	fFrequency[EQ_BANDS];
	
	double 	fBands[EQ_BANDS];
	double 	fPreAmp;	
	
};

#endif //__EQUALIZER_H__
