//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, OpenBeOS
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		AppServer.cpp
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//	Description:	main manager object for the app_server
//  
//------------------------------------------------------------------------------
#include "AppServer.h"
#include "ServerConfig.h"

AppServer::AppServer(void)
{
}

AppServer::~AppServer(void)
{
}

int32 AppServer::PollerThread(void *data)
{
	return 0;
}

int32 AppServer::PicassoThread(void *data)
{
	return 0;
}

thread_id AppServer::Run(void)
{
	return 0;
}

void AppServer::MainLoop(void)
{
}

bool AppServer::LoadDecorator(const char *path)
{
	return false;
}

void AppServer::DispatchMessage(int32 code, int8 *buffer)
{
}

void AppServer::Broadcast(int32 code)
{
}

void AppServer::HandleKeyMessage(int32 code, int8 *buffer)
{
}

int main( int argc, char** argv )
{
	// There can be only one....
	if(find_port(SERVER_PORT_NAME)!=B_NAME_NOT_FOUND)
		return -1;

	AppServer *app_server = new AppServer();
	app_server->Run();
	delete app_server;
	return 0;
}
