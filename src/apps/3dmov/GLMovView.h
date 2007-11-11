/* 
 * Copyright 2004-2007, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		François Revol <revol@free.fr>
 */

#ifndef _GLMOVVIEW_H
#define _GLMOVVIEW_H

#include <GLView.h>
#include <MediaFile.h>

class GLMovView: public BGLView
{
	public:
					GLMovView(BRect frame);
					~GLMovView();
	virtual void	AttachedToWindow();
	
	// for mouse control
	virtual void	MouseDown(BPoint where);
	virtual void	MouseUp(BPoint where);
	virtual void	MouseMoved(BPoint where, uint32 code, const BMessage *a_message);
	
	//
	virtual void	FileDropped(BPoint where, BMediaFile *file);
};

#endif /* _GLMOVVIEW_H */
