/*
 * Copyright 2004-2011 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *      Alexander von Gluck, kallisti5@unixzen.com
 */


#include "InterfaceHardwareView.h"
#include "NetworkSettings.h"

#include <LayoutBuilder.h>
#include <MenuItem.h>


InterfaceHardwareView::InterfaceHardwareView(BRect frame,
	NetworkSettings* settings)
	:
	BGroupView(B_VERTICAL),
	fSettings(settings)
{
	SetLayout(new BGroupLayout(B_VERTICAL));

	// TODO : Small graph of throughput?

	// TODO : Use strings instead of TextControls
	fStatusField = new BTextControl("Status:", NULL, NULL);
	fStatusField->SetEnabled(false);
	fMACField = new BTextControl("MAC Address:", NULL, NULL);
	fMACField->SetEnabled(false);
	fSpeedField = new BTextControl("Link Speed:", NULL, NULL);
	fSpeedField->SetEnabled(false);

	RevertFields();
		// Do the initial field population

	BLayoutBuilder::Group<>(this)
		.AddGrid()
			.AddTextControl(fStatusField, 0, 0, B_ALIGN_RIGHT)
			.AddTextControl(fMACField, 0, 1, B_ALIGN_RIGHT)
			.AddTextControl(fSpeedField, 0, 2, B_ALIGN_RIGHT)
		.End()
		.AddGlue()
		.SetInsets(B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING,
			B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING);
}


InterfaceHardwareView::~InterfaceHardwareView()
{

}


void
InterfaceHardwareView::AttachedToWindow()
{

}


void
InterfaceHardwareView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		default:
			BView::MessageReceived(message);
	}
}


status_t
InterfaceHardwareView::RevertFields()
{
	// Populate fields with current settings
	if (fSettings->HasLink())
		fStatusField->SetText("connected");
	else
		fStatusField->SetText("disconnected");

	// TODO : Find how to get link speed
	fSpeedField->SetText("100 Mb/s");

	return B_OK;
}


status_t
InterfaceHardwareView::SaveFields()
{

	return B_OK;
}

