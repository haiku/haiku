#include <MediaDefs.h>
#include <Locker.h>
#include <OS.h>

#include "MixerCore.h"
#include "MixerInput.h"
#include "MixerOutput.h"
#include "MixerSettings.h"
#include "debug.h"

#define SAVE_DELAY		8000000		// delay saving of settings for 8s
#define SAVE_RUNTIME	40000000	// stop save thread after 40s inactivity

MixerSettings::MixerSettings()
 :	fLocker(new BLocker),
	fSettingsDirty(false),
	fSettingsLastChange(0),
	fSaveThread(-1),
	fSaveThreadWaitSem(-1)
{
	Load();
}

MixerSettings::~MixerSettings()
{
	StopDeferredSave();
	if (fSettingsDirty)
		Save();
	delete fLocker;
}

bool
MixerSettings::AttenuateOutput()
{
	bool temp;
	fLocker->Lock();
	temp = fSettings.AttenuateOutput;
	fLocker->Unlock();
	return temp;
}

void
MixerSettings::SetAttenuateOutput(bool yesno)
{
	fLocker->Lock();
	fSettings.AttenuateOutput = yesno;
	fLocker->Unlock();
	StartDeferredSave();
}

bool
MixerSettings::NonLinearGainSlider()
{
	bool temp;
	fLocker->Lock();
	temp = fSettings.NonLinearGainSlider;
	fLocker->Unlock();
	return temp;
}

void
MixerSettings::SetNonLinearGainSlider(bool yesno)
{
	fLocker->Lock();
	fSettings.NonLinearGainSlider = yesno;
	fLocker->Unlock();
	StartDeferredSave();
}

bool
MixerSettings::UseBalanceControl()
{
	bool temp;
	fLocker->Lock();
	temp = fSettings.UseBalanceControl;
	fLocker->Unlock();
	return temp;
}

void
MixerSettings::SetUseBalanceControl(bool yesno)
{
	fLocker->Lock();
	fSettings.UseBalanceControl = yesno;
	fLocker->Unlock();
	StartDeferredSave();
}

bool
MixerSettings::AllowOutputChannelRemapping()
{
	bool temp;
	fLocker->Lock();
	temp = fSettings.AllowOutputChannelRemapping;
	fLocker->Unlock();
	return temp;
}

void
MixerSettings::SetAllowOutputChannelRemapping(bool yesno)
{
	fLocker->Lock();
	fSettings.AllowOutputChannelRemapping = yesno;
	fLocker->Unlock();
	StartDeferredSave();
}

bool
MixerSettings::AllowInputChannelRemapping()
{
	bool temp;
	fLocker->Lock();
	temp = fSettings.AllowInputChannelRemapping;
	fLocker->Unlock();
	return temp;
}

void
MixerSettings::SetAllowInputChannelRemapping(bool yesno)
{
	fLocker->Lock();
	fSettings.AllowInputChannelRemapping = yesno;
	fLocker->Unlock();
	StartDeferredSave();
}

int
MixerSettings::InputGainControls()
{
	int temp;
	fLocker->Lock();
	temp = fSettings.InputGainControls;
	fLocker->Unlock();
	return temp;
}

void
MixerSettings::SetInputGainControls(int value)
{
	fLocker->Lock();
	fSettings.InputGainControls = value;
	fLocker->Unlock();
	StartDeferredSave();
}

int
MixerSettings::ResamplingAlgorithm()
{
	int temp;
	fLocker->Lock();
	temp = fSettings.ResamplingAlgorithm;
	fLocker->Unlock();
	return temp;
}

void
MixerSettings::SetResamplingAlgorithm(int value)
{
	fLocker->Lock();
	fSettings.ResamplingAlgorithm = value;
	fLocker->Unlock();
	StartDeferredSave();
}

bool
MixerSettings::RefuseOutputFormatChange()
{
	bool temp;
	fLocker->Lock();
	temp = fSettings.RefuseOutputFormatChange;
	fLocker->Unlock();
	return temp;
}

void
MixerSettings::SetRefuseOutputFormatChange(bool yesno)
{
	fLocker->Lock();
	fSettings.RefuseOutputFormatChange = yesno;
	fLocker->Unlock();
	StartDeferredSave();
}

bool
MixerSettings::RefuseInputFormatChange()
{
	bool temp;
	fLocker->Lock();
	temp = fSettings.RefuseInputFormatChange;
	fLocker->Unlock();
	return temp;
}

void
MixerSettings::SetRefuseInputFormatChange(bool yesno)
{
	fLocker->Lock();
	fSettings.RefuseInputFormatChange = yesno;
	fLocker->Unlock();
	StartDeferredSave();
}

void
MixerSettings::SaveGain(MixerInput *input)
{
}

void
MixerSettings::LoadGain(MixerInput *input)
{
}

void
MixerSettings::SaveGain(MixerOutput *output)
{
}

void
MixerSettings::LoadGain(MixerOutput *output)
{
}

void
MixerSettings::SaveGainSetting(const char *name, uint32 channel_mask, const float *gain, int gain_count)
{
	fLocker->Lock();
	
	// XXX work 
	
	fLocker->Unlock();
	StartDeferredSave();
}

void
MixerSettings::LoadGainSetting(const char *name, uint32 channel_mask, float *gain, int gain_count)
{
	fLocker->Lock();
	
	// XXX work 
	
	fLocker->Unlock();
}
	
void
MixerSettings::SaveSetting(const char *name, int value)
{
	fLocker->Lock();
	
	// XXX work 
	
	fLocker->Unlock();
	StartDeferredSave();
}

void
MixerSettings::LoadSetting(const char *name, int *value, int default_value /* = 0 */)
{
	fLocker->Lock();
	
	// XXX work 
	
	fLocker->Unlock();
}

void
MixerSettings::Save()
{
	fLocker->Lock();
	TRACE("MixerSettings: SAVE!\n");

	
	// XXX work 
	
	fSettingsDirty = false;
	fLocker->Unlock();
}

void
MixerSettings::Load()
{
	fLocker->Lock();
	
	// XXX work
	
	fSettings.AttenuateOutput = true;
	fSettings.NonLinearGainSlider = true;
	fSettings.UseBalanceControl = false;
	fSettings.AllowOutputChannelRemapping = false;
	fSettings.AllowInputChannelRemapping = false;
	fSettings.InputGainControls = 0;
	fSettings.ResamplingAlgorithm = 0;
	fSettings.RefuseOutputFormatChange = false;
	fSettings.RefuseInputFormatChange = false;
	
	fLocker->Unlock();
}

void
MixerSettings::StartDeferredSave()
{
	fLocker->Lock();

	fSettingsDirty = true;
	fSettingsLastChange = system_time();
	
	if (fSaveThread < 0) {
		ASSERT(fSaveThreadWaitSem < 0);
		fSaveThreadWaitSem = create_sem(0, "save thread wait");
		if (fSaveThreadWaitSem < B_OK) {
			ERROR("MixerSettings: can't create semaphore\n");
			Save();
			fLocker->Unlock();
			return;
		}
		fSaveThread = spawn_thread(_save_thread_, "deferred settings save", 7, this);
		if (fSaveThread < B_OK) {
			ERROR("MixerSettings: can't spawn thread\n");
			delete_sem(fSaveThreadWaitSem);
			fSaveThreadWaitSem = -1;
			Save();
			fLocker->Unlock();
			return;
		}
	}
	fLocker->Unlock();
}

void
MixerSettings::StopDeferredSave()
{
	fLocker->Lock();

	if (fSaveThread > 0) {
		ASSERT(fSaveThreadWaitSem > 0);

		status_t unused;
		delete_sem(fSaveThreadWaitSem);
		wait_for_thread(fSaveThread, &unused);
		
		fSaveThread = -1;
		fSaveThreadWaitSem = -1;
	}
	
	fLocker->Unlock();
}

void
MixerSettings::SaveThread()
{
	bigtime_t timeout;
	status_t rv;
	
	TRACE("MixerSettings: save thread started\n");

	fLocker->Lock();
	timeout = fSettingsLastChange + SAVE_DELAY;
	fLocker->Unlock();
	
	for (;;) {
		rv = acquire_sem_etc(fSaveThreadWaitSem, 1, B_ABSOLUTE_TIMEOUT, timeout);
		if (rv == B_INTERRUPTED)
			continue;
		if (rv != B_TIMED_OUT && rv < B_OK)
			break;
		if (B_OK != fLocker->LockWithTimeout(200000))
			continue;
			
		TRACE("MixerSettings: save thread running\n");
			
		bigtime_t delta = system_time() - fSettingsLastChange;
			
		if (fSettingsDirty && delta > SAVE_DELAY) {
			Save();
		}

		if (delta > SAVE_RUNTIME) {
			fLocker->Unlock();
			break;
		}

		timeout = system_time() + SAVE_DELAY;
		fLocker->Unlock();
	}
	
	TRACE("MixerSettings: save thread ended\n");
}
	
int32
MixerSettings::_save_thread_(void *arg)
{
	static_cast<MixerSettings *>(arg)->SaveThread();
	return 0;
}
