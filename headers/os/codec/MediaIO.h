/*
 * Copyright 2016 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _MEDIA_IO_H
#define _MEDIA_IO_H


#include <DataIO.h>
#include <SupportDefs.h>


namespace BCodecKit {


enum media_io_flags {
	B_MEDIA_STREAMING = 0x00000001,

	B_MEDIA_MUTABLE_SIZE = 0x00000002,

	B_MEDIA_SEEK_FORWARD = 0x00000004,
	B_MEDIA_SEEK_BACKWARD = 0x00000008,
	B_MEDIA_SEEKABLE = B_MEDIA_SEEK_BACKWARD | B_MEDIA_SEEK_FORWARD
};


class BMediaIO : public BPositionIO {
public:
								BMediaIO();
	virtual						~BMediaIO();

	virtual void				GetFlags(int32* flags) const = 0;

private:
								BMediaIO(const BMediaIO&);
			BMediaIO&			operator=(const BMediaIO&);

	virtual	void				_ReservedMediaIO1();
	virtual void				_ReservedMediaIO2();
	virtual void				_ReservedMediaIO3();
	virtual void				_ReservedMediaIO4();
	virtual void				_ReservedMediaIO5();

			uint32				_reserved[5];
};


} // namespace BCodecKit


#endif	// _MEDIA_IO_H
