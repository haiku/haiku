/*
 * Copyright (c) 2001-2005, Haiku, Inc.
 * Copyright (c) 2003-4 Kian Duffy <myob@users.sourceforge.net>
 * Parts Copyright (C) 1998,99 Kazuho Okui and Takashi Murai. 
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Kian Duffy <myob@users.sourceforge.net>
 */
#ifndef	CURPOS_H_INCLUDED
#define	CURPOS_H_INCLUDED

#include <SupportDefs.h>

class CurPos
{
public:
			CurPos();
			CurPos(int32 X, int32 Y);
			CurPos(const CurPos& cp);
	
	void	Set(int32 X, int32 Y);
	
	CurPos	&operator= (const CurPos &from);
	CurPos	operator+  (const CurPos&) const;
	CurPos	operator-  (const CurPos&) const;
	bool	operator!= (const CurPos&) const;
	bool	operator== (const CurPos&) const;
	bool	operator>  (const CurPos&) const;
	bool	operator>= (const CurPos&) const;
	bool	operator<  (const CurPos&) const;
	bool	operator<= (const CurPos&) const;

	int32	x;
	int32	y;
};

#endif
