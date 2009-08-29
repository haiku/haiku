/*
 * Copyright 2009, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef	_DIGITAL_PORT_H
#define	_DIGITAL_PORT_H

#include <BeBuild.h>
#include <SupportDefs.h>

#include <stddef.h>


class BDigitalPort {
public:
							BDigitalPort();
	virtual					~BDigitalPort();

			status_t		Open(const char* portName);
			void			Close();
			bool			IsOpen();

			ssize_t			Read(uint8* buf);
			ssize_t			Write(uint8 value);

			status_t		SetAsOutput();
			bool			IsOutput();

			status_t		SetAsInput();
			bool			IsInput();

private:
	virtual	void		_ReservedDigitalPort1();
	virtual	void		_ReservedDigitalPort2();
	virtual	void		_ReservedDigitalPort3();

			int			fFd;
			bool		fIsInput;
			uint32		_fReserved[3];
};

#endif // _DIGITAL_PORT_H

