/*
  -----------------------------------------------------------------------------

	File:	GLMovWindow.h
	
	Date:	10/20/2004

	Description:	3DMov reloaded...
	
	Author:	François Revol.
	
	
	Copyright 2004, yellowTAB GmbH, All rights reserved.
	

  -----------------------------------------------------------------------------
*/
#ifndef _GLMOVWINDOW_H
#define _GLMOVWINDOW_H

#include <DirectWindow.h>

class GLMovView;

enum object_type {
	OBJ_CUBE,
	OBJ_SPHERE,
	OBJ_PULSE,
	OBJ_BOOK,
};

class GLMovWindow: public BDirectWindow
{
	public:
					GLMovWindow(GLMovView *view, BRect frame, const char *title);
					~GLMovWindow();
	virtual bool	QuitRequested();
	virtual void	MessageReceived(BMessage *message);
	virtual void	DirectConnected(direct_buffer_info *info);
	static GLMovWindow	*MakeWindow(object_type obj);

	private:
		GLMovView	*fGLView;
};

#endif /* _GLMOVWINDOW_H */
