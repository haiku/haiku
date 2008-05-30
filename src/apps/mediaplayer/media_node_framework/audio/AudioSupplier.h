/*	Copyright (c) 2000-2008, Ingo Weinhold <ingo_weinhold@gmx.de>,
 *	All Rights Reserved. Distributed under the terms of the MIT license.
 */

/*!	This class is an interface used by the AudioProducer to retreive the
	audio data to be played. */
#ifndef AUDIO_SUPPLIER_H
#define AUDIO_SUPPLIER_H


#include <MediaDefs.h>


class AudioProducer;

class AudioSupplier {
 public:
								AudioSupplier();
	virtual						~AudioSupplier();

	virtual	void				SetAudioProducer(AudioProducer* producer);

	virtual	status_t			GetFrames(void* buffer, int64 frameCount,
									bigtime_t startTime,
									bigtime_t endTime) = 0;

	virtual	void				SetFormat(const media_format& format) = 0;

	virtual	status_t			InitCheck() const;

 protected:
	AudioProducer*				fAudioProducer;
};


#endif	// AUDIO_SUPPLIER_H
