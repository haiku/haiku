/*
 * Copyright © 2000-2006 Ingo Weinhold <ingo_weinhold@gmx.de>
 * Copyright © 2008 Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT licensce.
 */

/*! This AudioReader just converts the source sample format (and byte order)
	into another one, e.g. LE short -> BE float. Frame rate and channel
	count remain unchanged.
*/

#ifndef AUDIO_FORMAT_CONVERTER_H
#define AUDIO_FORMAT_CONVERTER_H

#include "AudioReader.h"

class AudioFormatConverter : public AudioReader {
public:
								AudioFormatConverter(AudioReader* source,
									uint32 format, uint32 byte_order);
	virtual						~AudioFormatConverter();

	virtual	status_t			Read(void* buffer, int64 pos, int64 frames);

	virtual	status_t			InitCheck() const;

			AudioReader*		Source() const;

protected:
			AudioReader*		fSource;
};

#endif	// AUDIO_FORMAT_CONVERTER_H
