/*
 * Copyright 2009, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef	_D2A_H
#define	_D2A_H

#include <BeBuild.h>
#include <SupportDefs.h>

#include <stddef.h>

class BD2A {
public:
							BD2A();
	virtual					~BD2A();

			status_t		Open(const char* portName);
			void			Close();
			bool			IsOpen();

			ssize_t			Read(uint8* buf);
			ssize_t			Write(uint8 value);

private:
	virtual	void			_ReservedD2A1();
	virtual	void			_ReservedD2A2();
	virtual	void			_ReservedD2A3();

			int				fFd;
			uint32			_fReserved[3];
};

#endif //_D2A_H

