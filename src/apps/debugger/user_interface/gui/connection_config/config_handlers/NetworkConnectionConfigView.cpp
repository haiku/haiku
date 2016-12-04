/*
 * Copyright 2016, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#include "NetworkConnectionConfigView.h"

#include <LayoutBuilder.h>
#include <MenuField.h>
#include <TextControl.h>


enum {
	MSG_NET_CONFIG_INPUT_CHANGED = 'ncic'
};


NetworkConnectionConfigView::NetworkConnectionConfigView()
	:
	ConnectionConfigView("NetworkConnectionConfig"),
	fProtocolField(NULL),
	fHostInput(NULL),
	fPortInput(NULL)
{
}


NetworkConnectionConfigView::~NetworkConnectionConfigView()
{
}


void
NetworkConnectionConfigView::AttachedToWindow()
{
	ConnectionConfigView::AttachedToWindow();

	fHostInput->SetTarget(this);
	fPortInput->SetTarget(this);
}


void
NetworkConnectionConfigView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_NET_CONFIG_INPUT_CHANGED:
		{
			// TODO: implement
			break;
		}

		default:
		{
			ConnectionConfigView::MessageReceived(message);
			break;
		}
	}
}


status_t
NetworkConnectionConfigView::InitSpecific()
{
	fPortInput = new BTextControl("port_input", "Port:", "",
					new BMessage(MSG_NET_CONFIG_INPUT_CHANGED));

	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.AddGroup(B_HORIZONTAL)
			.Add((fHostInput = new BTextControl("host_input", "Host:", "",
					new BMessage(MSG_NET_CONFIG_INPUT_CHANGED))))
			.Add(fPortInput->CreateLabelLayoutItem())
			.Add(fPortInput->CreateTextViewLayoutItem())
		.End()
	.End();

	// since port numbers are limited to 5 digits, there's no need for
	// the port input to expand farther than that. Instead, let the
	// host field take the extra space.
	fPortInput->CreateTextViewLayoutItem()->SetExplicitMaxSize(BSize(
			be_plain_font->StringWidth("999999"), B_SIZE_UNSET));

	// TODO: init settings and protocol input

	return B_OK;
}


