/*
 * Copyright 2009, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _SOUND_H
#define _SOUND_H


#include <MediaDefs.h>

class BFile;
class BSoundPlayer;
struct entry_ref;

namespace BPrivate {
	class BTrackReader;
};


class BSound {
public:
								BSound(void* data, size_t size,
									const media_raw_audio_format& format,
									bool freeWhenDone = false);
								BSound(const entry_ref* soundFile,
									bool loadIntoMemory = false);

			status_t			InitCheck();
			BSound* 			AcquireRef();
			bool				ReleaseRef();
			int32				RefCount() const; // unreliable!

	virtual	bigtime_t			Duration() const;
	virtual	const media_raw_audio_format &Format() const;
	virtual	const void*			Data() const; // returns NULL for files
	virtual	off_t				Size() const;

	virtual	bool				GetDataAt(off_t offset,
									void* intoBuffer, size_t bufferSize,
									size_t* outUsed);

protected:
								BSound(const media_raw_audio_format& format);
	virtual	status_t			Perform(int32 code, ...);

private:
			friend	class DummyFriend;
	virtual						~BSound();

public:
	virtual	status_t			BindTo(BSoundPlayer* player,
									const media_raw_audio_format& format);
	virtual	status_t			UnbindFrom(BSoundPlayer* player);

private:
			status_t			_Reserved_Sound_0(void*);	// BindTo
			status_t			_Reserved_Sound_1(void*);	// UnbindFrom
	virtual	status_t			_Reserved_Sound_2(void*);
	virtual	status_t			_Reserved_Sound_3(void*);
	virtual	status_t			_Reserved_Sound_4(void*);
	virtual	status_t			_Reserved_Sound_5(void*);

private:
			void*				fData;
			size_t				fDataSize;
			BFile*				fFile;
			int32				fRefCount;
			status_t			fStatus;
			media_raw_audio_format fFormat;

			bool				fFreeWhenDone;
			bool				fReserved[3];

			BPrivate::BTrackReader* fTrackReader;
			uint32				fReserved2[18];
};

#endif // _SOUND_H
