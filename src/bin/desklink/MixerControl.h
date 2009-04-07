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
							MixerControl(int32 volumeWhich, float *value = NULL,
								const char **error = NULL);
							~MixerControl();

			void			SetVolume(float volume);
			void			ChangeVolumeBy(float value);

			float			Minimum() const { return fMin; }
			float			Maximum() const { return fMax; }

private:
			float			_GetVolume();
			void			_SetVolume(float volume);

			media_node*		fAudioMixerNode;
			BParameterWeb*	fParameterWeb;
			BContinuousParameter* fMixerParameter;
			float			fMin, fMax, fStep;
};

#endif	// MIXER_CONTROL_H
