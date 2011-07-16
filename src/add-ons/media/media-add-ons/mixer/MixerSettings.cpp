/*
 * Copyright 2003-2009 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Marcus Overhagen
 */


#include "MixerSettings.h"

#include <string.h>

#include <File.h>
#include <Locker.h>
#include <MediaDefs.h>
#include <OS.h>
#include <Path.h>

#include "MixerCore.h"
#include "MixerDebug.h"
#include "MixerInput.h"
#include "MixerOutput.h"


#define SAVE_DELAY		5000000		// delay saving of settings for 5s
#define SAVE_RUNTIME	30000000	// stop save thread after 30s inactivity

#define SETTINGS_VERSION ((int32)0x94251601)


MixerSettings::MixerSettings()
	:
	fLocker(new BLocker("mixer settings lock")),
 	fSettingsFile(0),
	fSettingsDirty(false),
	fSettingsLastChange(0),
	fSaveThread(-1),
	fSaveThreadWaitSem(-1),
	fSaveThreadRunning(false)
{
	Load();
}


MixerSettings::~MixerSettings()
{
	StopDeferredSave();
	if (fSettingsDirty)
		Save();
	delete fLocker;
	delete fSettingsFile;
}


void
MixerSettings::SetSettingsFile(const char *file)
{
	fLocker->Lock();
	delete fSettingsFile;
	fSettingsFile = new BPath(file);
	fLocker->Unlock();
	Load();
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
MixerSettings::SaveConnectionSettings(MixerInput *input)
{
	fLocker->Lock();
	int index = -1;

	// try to find matching name first
	for (int i = 0; i < MAX_INPUT_SETTINGS; i++) {
		if (fInputSetting[i].IsEmpty())
			continue;
		if (!strcmp(fInputSetting[i].FindString("name"), input->MediaInput().name)) {
			index = i;
			break;
		}
	}
	if (index == -1) {
		// try to find empty location
		for (int i = 0; i < MAX_INPUT_SETTINGS; i++) {
			if (fInputSetting[i].IsEmpty()) {
				index = i;
				break;
			}
		}
	}
	if (index == -1) {
		// find lru location
		index = 0;
		for (int i = 0; i < MAX_INPUT_SETTINGS; i++) {
			if (fInputSetting[i].FindInt64("lru") < fInputSetting[index].FindInt64("lru"))
				index = i;
		}
	}

	TRACE("SaveConnectionSettings: using entry %d\n", index);

	fInputSetting[index].MakeEmpty();
	fInputSetting[index].AddInt64("lru", system_time());
	fInputSetting[index].AddString("name", input->MediaInput().name);

	int count = input->GetInputChannelCount();
	fInputSetting[index].AddInt32("InputChannelCount", count);
	fInputSetting[index].AddBool("InputIsEnabled", input->IsEnabled());

	for (int i = 0; i < count; i++)
		fInputSetting[index].AddFloat("InputChannelGain", input->GetInputChannelGain(i));

	// XXX should save channel destinations and mixer channels

	fLocker->Unlock();
	StartDeferredSave();
}


void
MixerSettings::LoadConnectionSettings(MixerInput *input)
{
	fLocker->Lock();
	int index;
	for (index = 0; index < MAX_INPUT_SETTINGS; index++) {
		if (fInputSetting[index].IsEmpty())
			continue;
		if (!strcmp(fInputSetting[index].FindString("name"), input->MediaInput().name))
			break;
	}
	if (index == MAX_INPUT_SETTINGS) {
		TRACE("LoadConnectionSettings: entry not found\n");
		fLocker->Unlock();
		return;
	}

	TRACE("LoadConnectionSettings: found entry %d\n", index);

	int count = input->GetInputChannelCount();
	if (fInputSetting[index].FindInt32("InputChannelCount") == count) {
		for (int i = 0; i < count; i++)
			input->SetInputChannelGain(i, fInputSetting[index].FindFloat("InputChannelGain", i));
		input->SetEnabled(fInputSetting[index].FindBool("InputIsEnabled"));
	}

	// XXX should load channel destinations and mixer channels

	fInputSetting[index].ReplaceInt64("lru", system_time());
	fLocker->Unlock();
	StartDeferredSave();
}


void
MixerSettings::SaveConnectionSettings(MixerOutput *output)
{
	fLocker->Lock();

	fOutputSetting.MakeEmpty();

	int count = output->GetOutputChannelCount();
	fOutputSetting.AddInt32("OutputChannelCount", count);
	for (int i = 0; i < count; i++)
		fOutputSetting.AddFloat("OutputChannelGain", output->GetOutputChannelGain(i));
	fOutputSetting.AddBool("OutputIsMuted", output->IsMuted());

	// XXX should save channel sources and source gains

	fLocker->Unlock();
	StartDeferredSave();
}


void
MixerSettings::LoadConnectionSettings(MixerOutput *output)
{
	fLocker->Lock();

	int count = output->GetOutputChannelCount();
	if (fOutputSetting.FindInt32("OutputChannelCount") == count) {
		for (int i = 0; i < count; i++)
			output->SetOutputChannelGain(i, fOutputSetting.FindFloat("OutputChannelGain", i));
		output->SetMuted(fOutputSetting.FindBool("OutputIsMuted"));
	}

	// XXX should load channel sources and source gains
	fLocker->Unlock();
}


void
MixerSettings::Save()
{
	fLocker->Lock();
	// if we don't have a settings file, don't continue
	if (!fSettingsFile) {
		fLocker->Unlock();
		return;
	}
	TRACE("MixerSettings: SAVE!\n");

	BMessage msg;
	msg.AddInt32("version", SETTINGS_VERSION);
	msg.AddData("settings", B_RAW_TYPE, (void *)&fSettings, sizeof(fSettings));
	msg.AddMessage("output", &fOutputSetting);
	for (int i = 0; i < MAX_INPUT_SETTINGS; i++)
		msg.AddMessage("input", &fInputSetting[i]);

	char *buffer;
    size_t length;

    length = msg.FlattenedSize();
    buffer = new char [length];
    msg.Flatten(buffer, length);

	BFile file(fSettingsFile->Path(), B_READ_WRITE | B_CREATE_FILE);
	file.Write(buffer, length);

   	delete [] buffer;

	fSettingsDirty = false;
	fLocker->Unlock();
}


void
MixerSettings::Load()
{
	fLocker->Lock();
	// setup defaults
	fSettings.AttenuateOutput = true;
	fSettings.NonLinearGainSlider = true;
	fSettings.UseBalanceControl = false;
	fSettings.AllowOutputChannelRemapping = false;
	fSettings.AllowInputChannelRemapping = false;
	fSettings.InputGainControls = 0;
	fSettings.ResamplingAlgorithm = 1;
	fSettings.RefuseOutputFormatChange = true;
	fSettings.RefuseInputFormatChange = true;

	// if we don't have a settings file, don't continue
	if (!fSettingsFile) {
		fLocker->Unlock();
		return;
	}

	BFile file(fSettingsFile->Path(), B_READ_WRITE);
	off_t size = 0;
	file.GetSize(&size);
	if (size == 0) {
		fLocker->Unlock();
		TRACE("MixerSettings: no settings file\n");
		return;
	}

	char * buffer = new char[size];
	if (size != file.Read(buffer, size)) {
		delete [] buffer;
		fLocker->Unlock();
		TRACE("MixerSettings: can't read settings file\n");
		return;
	}

	BMessage msg;
	if (B_OK != msg.Unflatten(buffer)) {
		delete [] buffer;
		fLocker->Unlock();
		TRACE("MixerSettings: can't unflatten settings\n");
		return;
	}

	delete [] buffer;

	if (msg.FindInt32("version") != SETTINGS_VERSION) {
		fLocker->Unlock();
		TRACE("MixerSettings: settings have wrong version\n");
		return;
	}

	const void *data;
	ssize_t datasize = 0;

	msg.FindData("settings", B_RAW_TYPE, &data, &datasize);
	if (datasize != sizeof(fSettings)) {
		fLocker->Unlock();
		TRACE("MixerSettings: settings have wrong size\n");
		return;
	}
	memcpy((void *)&fSettings, data, sizeof(fSettings));

	msg.FindMessage("output", &fOutputSetting);
	for (int i = 0; i < MAX_INPUT_SETTINGS; i++)
		msg.FindMessage("input", i, &fInputSetting[i]);

	fLocker->Unlock();
}


void
MixerSettings::StartDeferredSave()
{
	fLocker->Lock();

	// if we don't have a settings file, don't save the settings
	if (!fSettingsFile) {
		fLocker->Unlock();
		return;
	}

	fSettingsDirty = true;
	fSettingsLastChange = system_time();

	if (fSaveThreadRunning) {
		fLocker->Unlock();
		return;
	}

	StopDeferredSave();

	ASSERT(fSaveThreadWaitSem < 0);
	fSaveThreadWaitSem = create_sem(0, "save thread wait");
	if (fSaveThreadWaitSem < B_OK) {
		ERROR("MixerSettings: can't create semaphore\n");
		Save();
		fLocker->Unlock();
		return;
	}
	ASSERT(fSaveThread < 0);
	fSaveThread = spawn_thread(_save_thread_, "Attack of the Killer Tomatoes", 7, this);
	if (fSaveThread < B_OK) {
		ERROR("MixerSettings: can't spawn thread\n");
		delete_sem(fSaveThreadWaitSem);
		fSaveThreadWaitSem = -1;
		Save();
		fLocker->Unlock();
		return;
	}
	resume_thread(fSaveThread);

	fSaveThreadRunning = true;
	fLocker->Unlock();
}


void
MixerSettings::StopDeferredSave()
{
	fLocker->Lock();

	if (fSaveThread >= 0) {
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
			fSaveThreadRunning = false;
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
