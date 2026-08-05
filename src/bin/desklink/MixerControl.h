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
#ifndef MIXER_CONTROL_H
#define MIXER_CONTROL_H


#include <MediaRoster.h>

class BContinuousParameter;
class BParameter;
class BParameterWeb;

// The volume which choices
#define VOLUME_USE_MIXER		0 // default
#define VOLUME_USE_PHYS_OUTPUT	1


class MixerControl {
public:
							MixerControl();
							~MixerControl();

			bool			Connect(float* _value = NULL, const char** _error = NULL);
			bool			IsConnected();

			void			SetVolumeWhich(int32 volumeWhich) { fVolumeWhich = volumeWhich; };
			int32			VolumeWhich() const { return fVolumeWhich; };

			void			SetBeep(bool beep) { fBeep = beep; };
			int32			Beep() const { return fBeep; };

			void			SetMuted(bool mute);
			bool			IsMuted();

			void			SetVolume(float volume);
			void			ChangeVolumeBy(float value);
			float			Volume() const;

			float			Minimum() const { return fMin; }
			float			Maximum() const { return fMax; }

			media_node		GainNode() { return fGainMediaNode; }
			media_node		MuteNode() { return fMuteMediaNode; }

private:
			void			_Disconnect();
			void			_LoadSettings();
			void			_SaveSettings();

			int32			fVolumeWhich;		// act on mixer or physical output
			bool			fBeep;				// beep on volume change (default true)
			media_node		fGainMediaNode;
			media_node		fMuteMediaNode;
			BParameterWeb*	fParameterWeb;
			BContinuousParameter* fMixerParameter;
			BParameter*		fMuteParameter;
			float			fMin;
			float			fMax;
			float			fStep;
			BMediaRoster*	fRoster;
};


#endif	// MIXER_CONTROL_H
