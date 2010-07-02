/*
 * Copyright 2009 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Author:
 *		Artur Wyszynski <harakash@gmail.com>
 */

#ifndef OPENGL_APP_H
#define OPENGL_APP_H


#include <Application.h>
#include <Catalog.h>
#include "OpenGLWindow.h"


class OpenGLApp : public BApplication {
public:
    OpenGLApp();
    virtual ~OpenGLApp();

    virtual void ReadyToRun();

private:
    OpenGLWindow* fWindow;
};

#endif /* OPENGL_APP_H_ */
