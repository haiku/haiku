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


#define MENU_AUTO_MESSAGE	'auto'
#define MENU_SWRAST_MESSAGE	'swrt'
#define MENU_SWPIPE_MESSAGE	'swpi'
#define MENU_SWLLVM_MESSAGE	'swll'


class OpenGLView : public BGroupView {
public:
    OpenGLView();
    virtual ~OpenGLView();
};

#endif /* OPENGL_VIEW_H */
