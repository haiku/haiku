/*
 * Copyright 2003-2009, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Jérôme Duval
 *		François Revol
 *		Axel Dörfler, axeld@pinc-software.de.
 */


#include "MixerControl.h"

#include <string.h>

#include <Debug.h>
#include <ParameterWeb.h>


MixerControl::MixerControl(int32 volumeWhich)
	:
	fVolumeWhich(volumeWhich),
	fGainMediaNode(media_node::null),
	fParameterWeb(NULL),
	fMixerParameter(NULL),
	fMin(0.0f),
	fMax(0.0f),
	fStep(0.0f)
{
}


MixerControl::~MixerControl()
{
	_Disconnect();
}


bool
MixerControl::Connect(int32 volumeWhich, float* _value, const char** _error)
{
	fVolumeWhich = volumeWhich;

	_Disconnect();

	bool retrying = false;

	status_t status = B_OK;
		// BMediaRoster::Roster() doesn't set it if all is ok
	const char* errorString = NULL;
	BMediaRoster* roster = BMediaRoster::Roster(&status);

retry:
	// Here we release the BMediaRoster once if we can't access the system
	// mixer, to make sure it really isn't there, and it's not BMediaRoster
	// that is messed up.
	if (retrying) {
		errorString = NULL;
		PRINT(("retrying to get a Media Roster\n"));
		/* BMediaRoster looks doomed */
		roster = BMediaRoster::CurrentRoster();
		if (roster) {
			roster->Lock();
			roster->Quit();
		}
		snooze(10000);
		roster = BMediaRoster::Roster(&status);
	}

	if (roster != NULL && status == B_OK) {
		switch (volumeWhich) {
			case VOLUME_USE_MIXER:
				status = roster->GetAudioMixer(&fGainMediaNode);
				break;
			case VOLUME_USE_PHYS_OUTPUT:
				status = roster->GetAudioOutput(&fGainMediaNode);
				break;
		}
		if (status == B_OK) {
			status = roster->GetParameterWebFor(fGainMediaNode, &fParameterWeb);
			if (status == B_OK) {
				// Finding the Mixer slider in the audio output ParameterWeb
				int32 numParams = fParameterWeb->CountParameters();
				BParameter* p = NULL;
				bool foundMixerLabel = false;
				for (int i = 0; i < numParams; i++) {
					p = fParameterWeb->ParameterAt(i);
					PRINT(("BParameter[%i]: %s\n", i, p->Name()));
					if (volumeWhich == VOLUME_USE_MIXER) {
						if (!strcmp(p->Kind(), B_MASTER_GAIN))
							break;
					} else if (volumeWhich == VOLUME_USE_PHYS_OUTPUT) {
						/* not all cards use the same name, and
						 * they don't seem to use Kind() == B_MASTER_GAIN
						 */
						if (!strcmp(p->Kind(), B_MASTER_GAIN))
							break;
						PRINT(("not MASTER_GAIN \n"));

						/* some audio card
						 */
						if (!strcmp(p->Name(), "Master"))
							break;
						PRINT(("not 'Master' \n"));

						/* some Ensonic card have all controls names 'Volume', so
						 * need to fint the one that has the 'Mixer' text label
						 */
						if (foundMixerLabel && !strcmp(p->Name(), "Volume"))
							break;
						if (!strcmp(p->Name(), "Mixer"))
							foundMixerLabel = true;
						PRINT(("not 'Mixer' \n"));
					}
#if 0
					//if (!strcmp(p->Name(), "Master")) {
					if (!strcmp(p->Kind(), B_MASTER_GAIN)) {
						for (; i < numParams; i++) {
							p = fParamWeb->ParameterAt(i);
							if (strcmp(p->Kind(), B_MASTER_GAIN))
								p = NULL;
							else
								break;
						}
						break;
					} else
						p = NULL;
#endif
					p = NULL;
				}
				if (p == NULL) {
					errorString = volumeWhich ? "Could not find the soundcard"
						: "Could not find the mixer";
				} else if (p->Type() != BParameter::B_CONTINUOUS_PARAMETER) {
					errorString = volumeWhich ? "Soundcard control unknown"
						: "Mixer control unknown";
				} else {
					fMixerParameter = static_cast<BContinuousParameter*>(p);
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
			}
		} else {
			if (!retrying) {
				retrying = true;
				goto retry;
			}
			errorString = volumeWhich ? "No Audio output" : "No Mixer";
		}
	} else {
		if (!retrying) {
			retrying = true;
			goto retry;
		}
		errorString = "No Media Roster";
	}

	if (status != B_OK)
		fGainMediaNode = media_node::null;

	if (errorString) {
		fprintf(stderr, "MixerControl: %s.\n", errorString);
		if (_error)
			*_error = errorString;
	}
	if (fMixerParameter == NULL && _value != NULL)
		*_value = 0;

	return errorString == NULL;
}


bool
MixerControl::Connected()
{
	return fGainMediaNode != media_node::null;
}


int32
MixerControl::VolumeWhich() const
{
	return fVolumeWhich;
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


void
MixerControl::_Disconnect()
{
	delete fParameterWeb;
	fParameterWeb = NULL;
	fMixerParameter = NULL;

	BMediaRoster* roster = BMediaRoster::CurrentRoster();
	if (roster != NULL && fGainMediaNode != media_node::null)
		roster->ReleaseNode(fGainMediaNode);

	fGainMediaNode = media_node::null;
}
