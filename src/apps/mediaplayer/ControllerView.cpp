/*
 * Controller.cpp - Media Player for the Haiku Operating System
 *
 * Copyright (C) 2006 Marcus Overhagen <marcus@overhagen.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */
#include <Message.h>
#include <stdio.h>
#include <string.h>

#include "ControllerView.h"

ControllerView::ControllerView(BRect frame, Controller *ctrl)
 :	BView(frame, "controller", B_FOLLOW_ALL, B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE)
 ,	fController(ctrl)
{
	SetViewColor(128,0,0);
}


ControllerView::~ControllerView()
{
}


void
ControllerView::ResizeToPreferred()
{
	ResizeTo(300 - 1, 60 - 1);
}


void
ControllerView::AttachedToWindow()
{
}	


void
ControllerView::Draw(BRect updateRect)
{
	BView::Draw(updateRect);
}


void
ControllerView::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		default:
			BView::MessageReceived(msg);
	}
}

