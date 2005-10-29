/********************************************************************************
/
/	File:		A2D.h
/
/	Description:	Analog-to-Digital converter class header.
/
/	Copyright 1996-98, Be Incorporated, All Rights Reserved.
/
********************************************************************************/


#ifndef	_A2D_H
#define	_A2D_H

#include <BeBuild.h>
#include <stddef.h>
#include <SupportDefs.h>

/* -----------------------------------------------------------------------*/
class BA2D {

public:
					BA2D();
virtual				~BA2D();

		status_t	Open(const char *portName);
		void		Close(void);
		bool		IsOpen(void);

		ssize_t		Read(ushort *buf);

/* -----------------------------------------------------------------------*/

private:
virtual	void		_ReservedA2D1();
virtual	void		_ReservedA2D2();
virtual	void		_ReservedA2D3();

		int			ffd;
		uint32		_fReserved[3];
};

#endif
