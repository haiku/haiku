/*
 * Copyright 2011, Rene Gollent, rene@gollent.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "MemoryView.h"


MemoryView::MemoryView(Team* team, Listener* listener)
	:
	BView("memoryView", B_WILL_DRAW | B_SUBPIXEL_PRECISE),
	fTeam(team),
	fListener(listener)
{
}


MemoryView::~MemoryView()
{
	UnsetListener();
}


/*static */ MemoryView*
MemoryView::Create(Team* team, Listener* listener)
{
	MemoryView* self = new MemoryView(team, listener);

	try {
		self->_Init();
	} catch(...) {
		delete self;
		throw;
	}

	return self;
}


void
MemoryView::UnsetListener()
{
	fListener = NULL;
}


void
MemoryView::SetTargetAddress(target_addr_t address)
{
	fTargetAddress = address;
	Invalidate();
}


void
MemoryView::TargetAddressChanged(target_addr_t address)
{
	SetTargetAddress(address);
}


void
MemoryView::TargetedByScrollView(BScrollView* scrollView)
{
	BView::TargetedByScrollView(scrollView);
}


void
MemoryView::Draw(BRect rect)
{
	BView::Draw(rect);
}


void
MemoryView::MessageReceived(BMessage* message)
{
	switch(message->what) {
		default:
			BView::MessageReceived(message);
			break;
	}
}

void
MemoryView::_Init()
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
}


MemoryView::Listener::~Listener()
{
}
