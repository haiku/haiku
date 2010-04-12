/*
 * Copyright © 2000-2006 Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT licensce.
 */
#ifndef AUDIO_READER_H
#define AUDIO_READER_H

#include <MediaDefs.h>

class AudioReader {
public:
								AudioReader();
								AudioReader(const media_format& format);
	virtual						~AudioReader();

	virtual	status_t			InitCheck() const;

			void				SetFormat(const media_format& format);
			const media_format&	Format() const;

	virtual bigtime_t			InitialLatency() const = 0;
	virtual	status_t			Read(void* buffer, int64 pos, int64 frames) = 0;

			void				SetOutOffset(int64 offset);
			int64				OutOffset() const;

			int64				FrameForTime(bigtime_t time) const;
			bigtime_t			TimeForFrame(int64 frame) const;

protected:
			void*				ReadSilence(void* buffer, int64 frames) const;
			void*				SkipFrames(void* buffer, int64 frames) const;
			void				ReverseFrames(void* buffer,
									int64 frames) const;

protected:
			media_format		fFormat;
			int64				fOutOffset;
};

#endif	// AUDIO_READER_H
