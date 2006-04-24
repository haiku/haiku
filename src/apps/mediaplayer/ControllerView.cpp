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
 :	TransportControlGroup(frame)
 ,	fController(ctrl)
{
}


ControllerView::~ControllerView()
{
}


void
ControllerView::AttachedToWindow()
{
	TransportControlGroup::AttachedToWindow();
}


void
ControllerView::Draw(BRect updateRect)
{
	TransportControlGroup::Draw(updateRect);
}


void
ControllerView::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		default:
			TransportControlGroup::MessageReceived(msg);
	}
}

// #pragma mark -


uint32
ControllerView::EnabledButtons()
{
	return 0xffffffff;
}


void
ControllerView::TogglePlaying()
{
	printf("ControllerView::TogglePlaying()\n");
}


void
ControllerView::Stop()
{
	printf("ControllerView::Stop()\n");
}


void
ControllerView::Rewind()
{
	// TODO: make it so this function is called repeatedly
	printf("ControllerView::Rewind()\n");
}


void
ControllerView::Forward()
{
	// TODO: make it so this function is called repeatedly
	printf("ControllerView::Forward()\n");
}


void
ControllerView::SkipBackward()
{
	printf("ControllerView::SkipBackward()\n");
}


void
ControllerView::SkipForward()
{
	printf("ControllerView::SkipForward()\n");
}


void
ControllerView::SetVolume(float value)
{
	printf("ControllerView::SetVolume(%.2f)\n", value);
}


void
ControllerView::ToggleMute()
{
	printf("ControllerView::ToggleMute()\n");
}

