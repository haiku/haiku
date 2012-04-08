/*
 * Copyright 2012, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Aaron Hill <serac@hillvisions.com>
 */
#ifndef _GLIFE_GLIFEGRID_H
#define _GLIFE_GLIFEGRID_H


#include <OS.h>


// GLifeGrid Class Declaration
class GLifeGrid
{
private:
	bool*		m_pbGrid;
	int32		m_iWidth;
	int32		m_iHeight;
	
	int32		Neighbors(int32, int32);
	
public:
	// Constructor & Destructor
				GLifeGrid(int32, int32);
				~GLifeGrid(void);
	// Public Methods
	void		Generation(void);
	// Accessor Methods
	bool		Occupied(int32, int32);
};


#endif /* _GLIFE_GLIFEGRID_H */
