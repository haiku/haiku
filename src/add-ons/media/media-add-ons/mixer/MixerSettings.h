#ifndef _MIXER_SETTINGS_H
#define _MIXER_SETTINGS_H

class MixerInput;
class MixerOutput;

class MixerSettings
{
public:
	MixerSettings();
	~MixerSettings();

	bool	AttenuateOutput();
	void	SetAttenuateOutput(bool yesno);

	bool	NonLinearGainSlider();
	void	SetNonLinearGainSlider(bool yesno);

	bool	UseBalanceControl();
	void	SetUseBalanceControl(bool yesno);

	bool	AllowOutputChannelRemapping();
	void	SetAllowOutputChannelRemapping(bool yesno);

	bool	AllowInputChannelRemapping();
	void	SetAllowInputChannelRemapping(bool yesno);

	int		InputGainControls();
	void	SetInputGainControls(int value);

	int		ResamplingAlgorithm();
	void	SetResamplingAlgorithm(int value);

	bool	RefuseOutputFormatChange();
	void	SetRefuseOutputFormatChange(bool yesno);

	bool	RefuseInputFormatChange();
	void	SetRefuseInputFormatChange(bool yesno);

	void	SaveGain(MixerInput *input);
	void	LoadGain(MixerInput *input);

	void	SaveGain(MixerOutput *output);
	void	LoadGain(MixerOutput *output);
	
protected:

	void	SaveGainSetting(const char *name, uint32 channel_mask, const float *gain, int gain_count);
	void	LoadGainSetting(const char *name, uint32 channel_mask, float *gain, int gain_count);

	void	SaveSetting(const char *name, int value);
	void	LoadSetting(const char *name, int *value, int default_value = 0);

	void	StartDeferredSave();
	void	StopDeferredSave();
	
	void	Save();
	void	Load();

	static int32 _save_thread_(void *arg);
	void 	SaveThread();
	
	BLocker		*fLocker;
	bool		fSettingsDirty;
	bigtime_t	fSettingsLastChange;
	
	thread_id	fSaveThread;
	sem_id		fSaveThreadWaitSem;

	struct settings
	{
		bool	AttenuateOutput;
		bool	NonLinearGainSlider;
		bool	UseBalanceControl;
		bool	AllowOutputChannelRemapping;
		bool	AllowInputChannelRemapping;
		int		InputGainControls;
		int		ResamplingAlgorithm;
		bool	RefuseOutputFormatChange;
		bool	RefuseInputFormatChange;
	};

	settings	fSettings;
};

#endif //_MIXER_SETTINGS_H
