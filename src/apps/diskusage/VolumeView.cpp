/*
 * Copyright (c) 2010 Philippe Saint-Pierre, stpere@gmail.com
 * All rights reserved. Distributed under the terms of the MIT license.
 *
 * Copyright (c) 1999 Mike Steed. You are free to use and distribute this software
 * as long as it is accompanied by it's documentation and this copyright notice.
 * The software comes with no warranty, etc.
 */


#include <Catalog.h>
#include <Box.h>
#include <Button.h>
#include <StringView.h>
#include <Volume.h>
#include <Path.h>

#include <LayoutBuilder.h>

#include "DiskUsage.h"
#include "MainWindow.h"
#include "PieView.h"
#include "StatusView.h"

#include "VolumeView.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Volume View"

const float kMinWinSize = 275.0;

VolumeView::VolumeView(const char* name, BVolume* volume)
	: BView(name, B_WILL_DRAW)
{
	SetLayout(new BGroupLayout(B_HORIZONTAL));
	fPieView = new PieView(volume);

	fPieView->SetExplicitMinSize(BSize(kMinWinSize, kMinWinSize));

	fStatusView = new StatusView();

	AddChild(BLayoutBuilder::Group<>(B_VERTICAL, 2)
		.Add(fPieView)
		.Add(fStatusView)
	);
}


VolumeView::~VolumeView()
{
}


void
VolumeView::SetRescanEnabled(bool enabled)
{
	fStatusView->SetRescanEnabled(enabled);
}


void
VolumeView::SetPath(BPath path)
{
	fPieView->SetPath(path);
	fStatusView->SetBtnLabel(B_TRANSLATE("Rescan"));
}


void
VolumeView::MessageReceived(BMessage* msg)
{
	switch(msg->what) {
		case kBtnRescan:
			fPieView->MessageReceived(msg);
			fStatusView->SetBtnLabel(B_TRANSLATE("Rescan"));
			break;

		default:
			BView::MessageReceived(msg);
	}
}


void
VolumeView::ShowInfo(const FileInfo* info)
{
	fStatusView->ShowInfo(info);
}
