/*
 * Copyright 2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Paweł Dziepak, pdziepak@quarnos.org
 */
#ifndef FILEHANDLE_H
#define FILEHANDLE_H


#include <string.h>

#include <SupportDefs.h>


#define NFS4_FHSIZE	128

struct Filehandle {
			uint8		fSize;
			uint8		fFH[NFS4_FHSIZE];

	inline				Filehandle();
	inline				Filehandle(const Filehandle& fh);
	inline	Filehandle&	operator=(const Filehandle& fh);
};


inline
Filehandle::Filehandle()
	:
	fSize(0)
{
}


inline
Filehandle::Filehandle(const Filehandle& fh)
	:
	fSize(fh.fSize)
{
	memcpy(fFH, fh.fFH, fSize);
}


inline Filehandle&
Filehandle::operator=(const Filehandle& fh)
{
	fSize = fh.fSize;
	memcpy(fFH, fh.fFH, fSize);
	return *this;
}


#endif	// FILEHANDLE_H

