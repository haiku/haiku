/*
 * Copyright 2009 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef OPENGL_WINDOW_H
#define OPENGL_WINDOW_H


#include <Window.h>

#include "OpenGLView.h"


class OpenGLWindow : public BWindow {
public:
								OpenGLWindow();
		virtual					~OpenGLWindow();

		virtual	bool			QuitRequested();
		virtual	void			MessageReceived(BMessage* message);

private:
				OpenGLView*		fView;
};


#endif	/* OPENGL_WINDOW_H */
