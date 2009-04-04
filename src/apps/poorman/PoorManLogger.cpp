/*
 * Copyright 2009 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Author(s):
 *		Ma Jie, china.majie at gmail
 */

#include "PoorManLogger.h"

#include <time.h>
#include <netinet/in.h>

#include <Messenger.h>
#include <Message.h>
#include <TypeConstants.h>

#include "PoorManApplication.h"
#include "PoorManWindow.h"

void
poorman_log(const char* msg, bool needTimeHeader,
	in_addr_t addr, rgb_color color)
{
	time_t now = time(NULL);
	
	PoorManWindow* window = static_cast<PoorManApplication*>(be_app)->GetPoorManWindow();

	if(!window->LogConsoleFlag() && !window->LogFileFlag())
		return;
	
	BMessenger messenger(window);
	BMessage message(MSG_LOG);
	
	if(message.AddString("cstring", msg) != B_OK)
		return;
	if(needTimeHeader){
		if(message.AddData("time_t", B_TIME_TYPE, &now, sizeof(time_t)) != B_OK)
			return;
	}
	if(addr != INADDR_NONE)
		message.AddData("in_addr_t", B_ANY_TYPE, &addr, sizeof(in_addr_t));
	
	if(color != BLACK)
		message.AddData("rgb_color", B_RGB_COLOR_TYPE, &color, sizeof(rgb_color));

	messenger.SendMessage(&message, (BHandler*)NULL, 1000000);
}
