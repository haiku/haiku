/*
 * Copyright 2007 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _MIXER_SETTINGS_H
#define _MIXER_SETTINGS_H


#include <Message.h>
#include <Path.h>

class BLocker;
class MixerInput;
class MixerOutput;


#define MAX_INPUT_SETTINGS	50

class MixerSettings {
	public:
										MixerSettings();
										~MixerSettings();
	
		void							SetSettingsFile(const char *file);

		bool							AttenuateOutput();
		void							SetAttenuateOutput(bool yesno);

		bool							NonLinearGainSlider();
		void							SetNonLinearGainSlider(bool yesno);

		bool							UseBalanceControl();
		void							SetUseBalanceControl(bool yesno);

		bool							AllowOutputChannelRemapping();
		void							SetAllowOutputChannelRemapping(bool yesno);

		bool							AllowInputChannelRemapping();
		void							SetAllowInputChannelRemapping(bool yesno);

		int								InputGainControls();
		void							SetInputGainControls(int value);

		int								ResamplingAlgorithm();
		void							SetResamplingAlgorithm(int value);

		bool							RefuseOutputFormatChange();
		void							SetRefuseOutputFormatChange(bool yesno);

		bool							RefuseInputFormatChange();
		void							SetRefuseInputFormatChange(bool yesno);

		void							SaveConnectionSettings(MixerInput *input);
		void							LoadConnectionSettings(MixerInput *input);

		void							SaveConnectionSettings(MixerOutput *output);
		void							LoadConnectionSettings(MixerOutput *output);
	
	protected:
		void							StartDeferredSave();
		void							StopDeferredSave();
	
		void							Save();
		void							Load();

		static int32 					_save_thread_(void *arg);
		void 							SaveThread();

		BLocker							*fLocker;
		BPath							*fSettingsFile;
		volatile bool					fSettingsDirty;
		volatile bigtime_t				fSettingsLastChange;
		volatile thread_id				fSaveThread;
		volatile sem_id					fSaveThreadWaitSem;
		volatile bool					fSaveThreadRunning;

		struct settings {
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
	
		volatile settings				fSettings;

		BMessage						fOutputSetting;
		BMessage						fInputSetting[MAX_INPUT_SETTINGS];
};

#endif	// _MIXER_SETTINGS_H
