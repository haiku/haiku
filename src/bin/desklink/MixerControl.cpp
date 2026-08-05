/*
 * Copyright 2003-2026, Haiku. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Jérôme Duval
 *		François Revol
 *		Axel Dörfler, axeld@pinc-software.de.
 *		Puck Meerburg, puck@puckipedia.nl
 *		Dario Casalinuovo, b.vitruvio@gmail.com
 *		John Scipione, jscipione@gmail.com
 */


#include "MixerControl.h"

#include <string.h>

#include <Debug.h>
#include <File.h>
#include <FindDirectory.h>
#include <ParameterWeb.h>
#include <Path.h>


static const char* kSettingsFile = "x-vnd.Haiku-desklink";


MixerControl::MixerControl()
	:
	fVolumeWhich(VOLUME_USE_MIXER),
	fBeep(true),
	fGainMediaNode(media_node::null),
	fMuteMediaNode(media_node::null),
	fParameterWeb(NULL),
	fMixerParameter(NULL),
	fMuteParameter(NULL),
	fMin(0.0f),
	fMax(0.0f),
	fStep(0.0f),
	fRoster(NULL)
{
	fRoster = BMediaRoster::Roster();

	_LoadSettings();
}


MixerControl::~MixerControl()
{
	_SaveSettings();
	_Disconnect();
}


bool
MixerControl::Connect(float* _value, const char** _error)
{
	_Disconnect();

	status_t status = B_OK;
	const char* errorString = NULL;
	if (fRoster == NULL)
		fRoster = BMediaRoster::Roster(&status);

	if (BMediaRoster::IsRunning() && fRoster != NULL && status == B_OK) {
		switch (fVolumeWhich) {
			case VOLUME_USE_MIXER:
				status = fRoster->GetAudioMixer(&fGainMediaNode);
				break;

			case VOLUME_USE_PHYS_OUTPUT:
				status = fRoster->GetAudioOutput(&fGainMediaNode);
				break;
		}

		if (status == B_OK) {
			status = fRoster->GetParameterWebFor(fGainMediaNode, &fParameterWeb);
			if (status == B_OK) {
				// Finding the Mixer slider in the audio output ParameterWeb
				int32 numParams = fParameterWeb->CountParameters();
				BParameter* param = NULL;
				bool foundMixerLabel = false;
				for (int i = 0; i < numParams; i++) {
					param = fParameterWeb->ParameterAt(i);

					// assume the mute preceeding master gain control
					if (strcmp(param->Kind(), B_MUTE) == 0) {
						fMuteParameter = param;
						fMuteMediaNode = fMuteParameter->Web()->Node();
					}

					PRINT(("BParameter[%i]: %s\n", i, param->Name()));
					if (fVolumeWhich == VOLUME_USE_MIXER) {
						if (strcmp(param->Kind(), B_MASTER_GAIN) == 0)
							break;
					} else if (fVolumeWhich == VOLUME_USE_PHYS_OUTPUT) {
						/* not all cards use the same name, and
						 * they don't seem to use Kind() == B_MASTER_GAIN
						 */
						if (strcmp(param->Kind(), B_MASTER_GAIN) == 0)
							break;

						PRINT(("not MASTER_GAIN \n"));

						/* some audio card
						 */
						if (strcmp(param->Name(), "Master") == 0)
							break;
						PRINT(("not 'Master' \n"));

						/* some Ensonic card have all controls names 'Volume', so
						 * need to fint the one that has the 'Mixer' text label
						 */
						if (foundMixerLabel && strcmp(param->Name(), "Volume") == 0)
							break;

						if (strcmp(param->Name(), "Mixer") == 0)
							foundMixerLabel = true;

						PRINT(("not 'Mixer' \n"));
					}
#if 0
					//if (strcmp(param->Name(), "Master") == 0) {
					if (strcmp(param->Kind(), B_MASTER_GAIN) == 0) {
						for (; i < numParams; i++) {
							param = fParameterWeb->ParameterAt(i);
							if (strcmp(param->Kind(), B_MASTER_GAIN) == 0)
								break;
							else
								param = NULL;
						}
						break;
					} else {
						param = NULL;
					}
#endif
					param = NULL;
				}

				if (param == NULL) {
					errorString = fVolumeWhich
						? "Could not find the soundcard"
						: "Could not find the mixer";
				} else if (param->Type() != BParameter::B_CONTINUOUS_PARAMETER) {
					errorString = fVolumeWhich
						? "Soundcard control unknown"
						: "Mixer control unknown";
				} else {
					fMixerParameter = static_cast<BContinuousParameter*>(param);
					fMin = fMixerParameter->MinValue();
					fMax = fMixerParameter->MaxValue();
					fStep = fMixerParameter->ValueStep();

					if (_value != NULL) {
						float volume;
						bigtime_t lastChange;
						size_t size = sizeof(float);
						fMixerParameter->GetValue(&volume, &size, &lastChange);

						*_value = volume;
					}
				}
			} else {
				errorString = "No parameter web";
				fParameterWeb = NULL;
			}
		} else {
			errorString = fVolumeWhich
				? "No Audio output"
				: "No Mixer";
		}
	} else {
		errorString = "Media services not running";
	}

	if (status != B_OK) {
		_Disconnect();
		fMuteMediaNode = media_node::null;
	}

	if (errorString != NULL) {
		fprintf(stderr, "MixerControl: %s.\n", errorString);
		if (_error != NULL)
			*_error = errorString;
	}

	if (fMixerParameter == NULL && _value != NULL)
		*_value = 0;

	return errorString == NULL;
}


bool
MixerControl::IsConnected()
{
	return fGainMediaNode != media_node::null;
}


void
MixerControl::SetMuted(bool mute)
{
	if (fMuteParameter == NULL)
		return;

	int32 muted = mute ? 1 : 0;
	fMuteParameter->SetValue(&muted, sizeof(int32), system_time());
}


bool
MixerControl::IsMuted()
{
	if (fMuteParameter == NULL)
		return false;

	int32 muted = 0;
	bigtime_t lastChange = 0;
	size_t size = sizeof(int32);
	fMuteParameter->GetValue(&muted, &size, &lastChange);
	return muted != 0;
}


void
MixerControl::SetVolume(float volume)
{
	if (fMixerParameter == NULL)
		return;

	if (volume < fMin)
		volume = fMin;
	else if (volume > fMax)
		volume = fMax;

	if (volume != Volume())
		fMixerParameter->SetValue(&volume, sizeof(float), system_time());
}


void
MixerControl::ChangeVolumeBy(float value)
{
	if (fMixerParameter == NULL || value == 0.0f)
		return;

	float volume = Volume();
	SetVolume(volume + value);
}


float
MixerControl::Volume() const
{
	if (fMixerParameter == NULL)
		return 0.0f;

	float volume = 0;
	bigtime_t lastChange;
	size_t size = sizeof(float);
	fMixerParameter->GetValue(&volume, &size, &lastChange);

	return volume;
}


void
MixerControl::_Disconnect()
{
	delete fParameterWeb;
	fParameterWeb = NULL;
	fMixerParameter = NULL;

	if (fRoster == NULL)
		fRoster = BMediaRoster::Roster();

	if (fRoster != NULL && fGainMediaNode != media_node::null)
		fRoster->ReleaseNode(fGainMediaNode);

	fGainMediaNode = media_node::null;
}


void
MixerControl::_LoadSettings()
{
	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path, false) < B_OK)
		return;

	path.Append(kSettingsFile);

	BFile settings(path.Path(), B_READ_ONLY);
	if (settings.InitCheck() != B_OK)
		return;

	BMessage message;
	if (message.Unflatten(&settings) != B_OK)
		return;

	int32 volumeWhich;
	if (message.FindInt32("volwhich", &volumeWhich) == B_OK)
		SetVolumeWhich(volumeWhich);

	bool dontBeep;
	if (message.FindBool("dontbeep", &dontBeep) == B_OK)
		SetBeep(!dontBeep);
}


void
MixerControl::_SaveSettings()
{
	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path, false) != B_OK)
		return;

	path.Append(kSettingsFile);

	BFile settings(path.Path(), B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	if (settings.InitCheck() != B_OK)
		return;

	BMessage message('CNFG');
	message.AddInt32("volwhich", VolumeWhich());
	message.AddBool("dontbeep", !Beep());

	ssize_t size = 0;
	message.Flatten(&settings, &size);
}
