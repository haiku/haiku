/*
 * Copyright 2009 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Author:
 *		Artur Wyszynski <harakash@gmail.com>
 */

#include <Locale.h>

#include "OpenGLApp.h"
#include "OpenGLWindow.h"


OpenGLApp::OpenGLApp()
    : BApplication("application/x-vnd.Haiku-3DRendering")
{
}


OpenGLApp::~OpenGLApp()
{
}


void
OpenGLApp::ReadyToRun()
{
    fWindow = new OpenGLWindow();
    fWindow->Show();
}


int main(int argc, const char** argv)
{
    OpenGLApp app;
    app.Run();
    return 0;
}

