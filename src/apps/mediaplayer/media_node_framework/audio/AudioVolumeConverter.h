/*
 * Copyright © 2008 Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT licensce.
 */

/*! This AudioReader just filters the volume. It depends on floating point
 *	audio format.
*/

#ifndef AUDIO_VOLUME_CONVERTER_H
#define AUDIO_VOLUME_CONVERTER_H

#include "AudioReader.h"

class AudioVolumeConverter : public AudioReader {
public:
								AudioVolumeConverter(AudioReader* source,
									float volume = 1.0);
	virtual						~AudioVolumeConverter();

	virtual	status_t			Read(void* buffer, int64 pos, int64 frames);

	virtual	status_t			InitCheck() const;

			AudioReader*		Source() const;

			void				SetVolume(float volume);
			float				Volume() const;

protected:
			AudioReader*		fSource;
			float				fVolume;
			float				fPreviousVolume;
};

#endif	// AUDIO_VOLUME_CONVERTER_H
