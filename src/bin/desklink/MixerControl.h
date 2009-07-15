/*
 * Copyright 2003-2009, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Jérôme Duval
 *		François Revol
 */
#ifndef MIXER_CONTROL_H
#define MIXER_CONTROL_H


#include <MediaRoster.h>

class BParameterWeb;
class BContinuousParameter;


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

			float			Minimum() const { return fMin; }
			float			Maximum() const { return fMax; }

			media_node		GainNode() { return fGainMediaNode; }

private:
			void			_Disconnect();

			int32			fVolumeWhich;
			media_node		fGainMediaNode;
			BParameterWeb*	fParameterWeb;
			BContinuousParameter* fMixerParameter;
			float			fMin;
			float			fMax;
			float			fStep;
};

#endif	// MIXER_CONTROL_H
