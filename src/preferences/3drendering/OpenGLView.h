/*
 * Copyright 2009-2010 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Author:
 *		Alex Wilson <yourpalal2@gmail.com>
 *		Artur Wyszynski <harakash@gmail.com>
 */

#ifndef OPENGL_VIEW_H
#define OPENGL_VIEW_H


#include <GroupView.h>

class OpenGLView : public BGroupView {
public:
    OpenGLView();
    virtual ~OpenGLView();
};

#endif /* OPENGL_VIEW_H */
