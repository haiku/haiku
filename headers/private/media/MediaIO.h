/*
 * Copyright 2016 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _MEDIA_IO_H
#define _MEDIA_IO_H


#include <DataIO.h>
#include <SupportDefs.h>


class BMediaIO : public BPositionIO {
public:
								BMediaIO();
	virtual						~BMediaIO();

	virtual bool				IsSeekable() const = 0;
	virtual	bool				IsEndless() const = 0;

private:
								BMediaIO(const BMediaIO&);
			BMediaIO&			operator=(const BMediaIO&);

	virtual	void				_ReservedMediaIO1();
	virtual void				_ReservedMediaIO2();
	virtual void				_ReservedMediaIO3();
	virtual void				_ReservedMediaIO4();
	virtual void				_ReservedMediaIO5();

			uint32				_reserved[1];
};

#endif	// _MEDIA_IO_H
