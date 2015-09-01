/*
 * Copyright 2003-2013, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Jérôme Duval
 *		François Revol
 *		Puck Meerburg, puck@puckipedia.nl
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
							MixerControl(int32 volumeWhich = VOLUME_USE_MIXER);
							~MixerControl();

			bool			Connect(int32 volumeWhich, float* _value = NULL,
								const char** _error = NULL);
			bool			Connected();

			int32			VolumeWhich() const;
			float			Volume() const;

			void			SetVolume(float volume);
			void			ChangeVolumeBy(float value);

			void			SetMute(bool muted);
			bool			Mute();

			float			Minimum() const { return fMin; }
			float			Maximum() const { return fMax; }

			media_node		GainNode() { return fGainMediaNode; }

private:
			void			_Disconnect();

			int32			fVolumeWhich;
			media_node		fGainMediaNode;
			BParameterWeb*	fParameterWeb;
			BContinuousParameter* fMixerParameter;
			BParameter*		fMuteParameter;
			float			fMin;
			float			fMax;
			float			fStep;
			BMediaRoster*	fRoster;
};

#endif	// MIXER_CONTROL_H
