/*
 * Copyright 2009 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Author:
 *		Artur Wyszynski <harakash@gmail.com>
 */

#ifndef OPENGL_VIEW_H
#define OPENGL_VIEW_H


#include <GLView.h>
#include <View.h>

class OpenGLView : public BView {
public:
    OpenGLView();
    virtual ~OpenGLView();

	virtual	void MessageReceived(BMessage* message);
	virtual	void AttachedToWindow();
	virtual	void DetachedFromWindow();

private:
    BGLView* fGLView;
};

#endif /* OPENGL_VIEW_H */
