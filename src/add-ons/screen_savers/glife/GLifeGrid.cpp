/*
 * Copyright 2012, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Aaron Hill <serac@hillvisions.com>
 */


#include <math.h>
#include <stdlib.h>
#include <SupportDefs.h>

#include "GLifeGrid.hpp"


// ------------------------------------------------------
//  GLifeGrid Class Neighbors Definition
inline int32 GLifeGrid::Neighbors( int32 iRow, int32 iColumn )
{
	return ( Occupied( iRow - 1, iColumn - 1 ) ? 1 : 0 ) + 
		   ( Occupied( iRow - 1, iColumn     ) ? 1 : 0 ) + 
		   ( Occupied( iRow - 1, iColumn + 1 ) ? 1 : 0 ) +
		   ( Occupied( iRow    , iColumn - 1 ) ? 1 : 0 ) + 
		   ( Occupied( iRow    , iColumn + 1 ) ? 1 : 0 ) + 
		   ( Occupied( iRow + 1, iColumn - 1 ) ? 1 : 0 ) + 
		   ( Occupied( iRow + 1, iColumn     ) ? 1 : 0 ) + 
		   ( Occupied( iRow + 1, iColumn + 1 ) ? 1 : 0 );
}

// ------------------------------------------------------
//  GLifeGrid Class Constructor Definition
GLifeGrid::GLifeGrid( int32 iWidth, int32 iHeight )
	:	m_iWidth( iWidth ), m_iHeight( iHeight )
{
	m_pbGrid = new bool[iWidth * iHeight];	
	
	// Randomize new grid
	for( int32 iRow = 0; iRow < iHeight; ++iRow )
	{
		for( int32 iColumn = 0; iColumn < iWidth; ++iColumn )
		{
			// TODO: Allow for customized randomization level
			m_pbGrid[ ( iRow * iWidth ) + iColumn ] = ( (rand() % 6 ) == 0 );
		}
	}
}

// ------------------------------------------------------
//  GLifeGrid Class Destructor Definition
GLifeGrid::~GLifeGrid( void )
{
	delete m_pbGrid;
}

// ------------------------------------------------------
//  GLifeGrid Class Generation Definition
void GLifeGrid::Generation( void )
{
	bool* pbTemp = new bool[m_iWidth * m_iHeight];

	for( int32 iRow = 0; iRow < m_iHeight; ++iRow )
	{
		for( int32 iColumn = 0; iColumn < m_iWidth; ++iColumn )
		{
			int32 iNum = Neighbors( iRow, iColumn );
			pbTemp[ ( iRow * m_iWidth ) + iColumn ] =
				( ( iNum == 3 ) ||
				( Occupied( iRow, iColumn ) && iNum >= 2 && iNum <= 3 ) );
		}
	}
	
	// Swap grids
	delete m_pbGrid;
	m_pbGrid = pbTemp;
}

// ------------------------------------------------------
//  GLifeGrid Class Occupied Definition
bool GLifeGrid::Occupied( int32 iRow, int32 iColumn )
{
	int32 iNewRow = ( iRow % m_iHeight ), iNewColumn = ( iColumn % m_iWidth );
	while( iNewRow < 0 ) iNewRow += m_iHeight;
	while( iNewColumn < 0 ) iNewColumn += m_iWidth;
	return m_pbGrid[ ( iNewRow * m_iWidth ) + iNewColumn ];
}