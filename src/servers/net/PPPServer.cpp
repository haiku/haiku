/*
 * Copyright 2004-2005, Waldemar Kornewald <wkornew@gmx.net>
 * Distributed under the terms of the MIT License.
 */

#include "PPPServer.h"
#include <Application.h>


PPPServer::PPPServer()
	: BHandler("PPPServer"),
	fListener(this)
{
	be_app->AddHandler(this);

	fListener.WatchManager();

	InitInterfaces();
}


PPPServer::~PPPServer()
{
	UninitInterfaces();

	fListener.StopWatchingManager();

	be_app->RemoveHandler(this);
}


void
PPPServer::MessageReceived(BMessage *message)
{
	switch(message->what) {
		case PPP_REPORT_MESSAGE:
			HandleReportMessage(message);
		break;

		default:
			BHandler::MessageReceived(message);
	}
}


void
PPPServer::InitInterfaces()
{
	// TODO: create one ConnectionRequestWindow per interface
}


void
PPPServer::UninitInterfaces()
{
	// TODO: delete all ConnectionRequestWindows
}


void
PPPServer::HandleReportMessage(BMessage *message)
{
	ppp_interface_id id;
	if (message->FindInt32("interface", reinterpret_cast<int32*>(&id)) != B_OK)
		return;

	int32 type, code;
	message->FindInt32("type", &type);
	message->FindInt32("code", &code);

	if (type == PPP_MANAGER_REPORT && code == PPP_REPORT_INTERFACE_CREATED)
		CreateConnectionRequestWindow(id);
}


void
PPPServer::CreateConnectionRequestWindow(ppp_interface_id id)
{
	// TODO: create window, register window as report receiver for the interface
	// XXX: if a window for that ID exists then only register it as report receiver
}
