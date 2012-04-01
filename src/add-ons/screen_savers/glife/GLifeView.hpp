/*
 * Copyright 2012, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Aaron Hill <serac@hillvisions.com>
 */
#ifndef _GLIFE_GLIFEVIEW_HPP
#define _GLIFE_GLIFEVIEW_HPP


#include <GLView.h>
#include "GLifeState.hpp"


// GLifeView Class Declaration
class GLifeView : public BGLView {
public:
	// Constructor & Destructor
						GLifeView( BRect, char*, ulong, ulong, GLifeState* );
						~GLifeView( void );
				
	// Public Methods
			void		AttachedToWindow( void );
	virtual	void		Draw(BRect updateRect);
			void		Advance( void );

private:
			GLifeState*	m_pglsState;
			GLifeGrid*	m_pglgGrid;

			GLfloat		m_glfDelta;
			int32		m_iStep;
};

#endif /* _GLIFE_GLIFEVIEW_HPP */
