
enum speex_mode {
	automatic_band = -1,
	narrow_band, 
	wide_band,
	ultra_wide_band,
};

enum speex_channels {
	automatic_channels = -1, 
	mono_channels,
	stereo_channels,
};

class SpeexSettings {
private:
	SpeexSettings();
public:
	static bool PerceptualPostFilter(void);
	static speex_mode PreferredBand(void);
	static speex_channels PreferredChannels(void);
	// if non-zero, specifies a sampling rate in Hertz
	static float SamplingRate(void);

private:
	static bool perceptual_post_filter;
	static speex_mode preferred_band;
	static speex_channels preferred_channels;
	static float sampling_rate;
};
