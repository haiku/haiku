/*
 * Copyright 2009, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef	_A2D_H
#define	_A2D_H

#include <BeBuild.h>
#include <SupportDefs.h>

#include <stddef.h>

class BA2D {
public:
							BA2D();
	virtual					~BA2D();

			status_t		Open(const char* portName);
			void			Close();
			bool			IsOpen();

			ssize_t			Read(ushort* buf);

private:
	virtual	void			_ReservedA2D1();
	virtual	void			_ReservedA2D2();
	virtual	void			_ReservedA2D3();

			int				fFd;
			uint32			_fReserved[3];
};

#endif // _A2D_H

