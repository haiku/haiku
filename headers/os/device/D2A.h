/********************************************************************************
/
/	File:		D2A.h
/
/	Description:	Digital-To-Analog converter class header.
/
/	Copyright 1996-98, Be Incorporated, All Rights Reserved.
/
********************************************************************************/


#ifndef	_D2A_H
#define	_D2A_H

#include <BeBuild.h>
#include <stddef.h>
#include <SupportDefs.h>

/* -----------------------------------------------------------------------*/
class BD2A {

public:
					BD2A();
virtual				~BD2A();

		status_t	Open(const char *portName);
		void		Close(void);
		bool		IsOpen(void);

		ssize_t		Read(uint8 *buf);
		ssize_t		Write(uint8 value);

/* -----------------------------------------------------------------------*/

private:

virtual	void		_ReservedD2A1();
virtual	void		_ReservedD2A2();
virtual	void		_ReservedD2A3();

		int			ffd;
		uint32		_fReserved[3];
};

#endif
