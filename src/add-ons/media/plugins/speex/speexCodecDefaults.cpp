#include "speexCodecDefaults.h"

bool SpeexSettings::perceptual_post_filter = true;
speex_mode SpeexSettings::preferred_band = automatic_band;
speex_channels SpeexSettings::preferred_channels = automatic_channels;
float SpeexSettings::sampling_rate = 0;

SpeexSettings::SpeexSettings()
{
}

/* static */ bool
SpeexSettings::PerceptualPostFilter(void)
{
	return perceptual_post_filter;
}

/* static */ speex_mode
SpeexSettings::PreferredBand(void)
{
	return preferred_band;
}

/* static */ speex_channels
SpeexSettings::PreferredChannels(void)
{
	return preferred_channels;
}

// if non-zero, specifies a sampling rate in Hertz
/* static */ float
SpeexSettings::SamplingRate(void)
{
	return sampling_rate;
}
