/*
 * Copyright 2016 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _ADAPTER_IO_H
#define _ADAPTER_IO_H


#include <MediaIO.h>
#include <OS.h>
#include <SupportDefs.h>


class RWLocker;


namespace BCodecKit {


class BAdapterIO;
class RelativePositionIO;


class BInputAdapter {
public:
	virtual	ssize_t					Write(const void* buffer, size_t size);

private:
	friend class BAdapterIO;

									BInputAdapter(BAdapterIO* io);
	virtual							~BInputAdapter();

			BAdapterIO*				fIO;

	virtual	void					_ReservedInputAdapter1();
	virtual void					_ReservedInputAdapter2();

			uint32					_reserved[2];
};


class BAdapterIO : public BMediaIO {
public:
									BAdapterIO(
										int32 flags,
										bigtime_t timeout);
	virtual							~BAdapterIO();

	virtual void					GetFlags(int32* flags) const;

	virtual	ssize_t					ReadAt(off_t position, void* buffer,
										size_t size);
	virtual	ssize_t					WriteAt(off_t position,
										const void* buffer, size_t size);

	virtual	off_t					Seek(off_t position, uint32 seekMode);
	virtual off_t					Position() const;

	virtual	status_t				SetSize(off_t size);
	virtual	status_t				GetSize(off_t* size) const;

	virtual status_t				Open();

	virtual bool					IsRunning() const;

			void					SeekCompleted();
			status_t				SetBuffer(BPositionIO* buffer);

			BInputAdapter*			BuildInputAdapter();

protected:
	friend class BInputAdapter;

	virtual	status_t				SeekRequested(off_t position);

			ssize_t					BackWrite(const void* buffer, size_t size);

private:
			status_t				_EvaluateWait(off_t pos, off_t size);

			int32					fFlags;

			RelativePositionIO*		fBuffer;
			off_t					fTotalSize;
			bool					fOpened;
			sem_id					fSeekSem;

			BInputAdapter*			fInputAdapter;

									BAdapterIO(const BAdapterIO&);
			BAdapterIO&				operator=(const BAdapterIO&);

	virtual	void					_ReservedAdapterIO1();
	virtual void					_ReservedAdapterIO2();
	virtual void					_ReservedAdapterIO3();
	virtual void					_ReservedAdapterIO4();
	virtual void					_ReservedAdapterIO5();

			uint32					_reserved[5];
};


} // namespace BCodecKit


#endif	// _ADAPTER_IO_H
