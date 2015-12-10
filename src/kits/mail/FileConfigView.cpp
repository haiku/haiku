/*
 * Copyright 2004-2012, Haiku, Inc. All rights reserved.
 * Copyright 2001 Dr. Zoidberg Enterprises. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */


//! A file configuration view for filters


#include <FileConfigView.h>

#include <stdio.h>

#include <Button.h>
#include <Catalog.h>
#include <GroupLayout.h>
#include <MailSettingsView.h>
#include <Message.h>
#include <Path.h>
#include <String.h>
#include <TextControl.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "MailKit"


static const uint32 kMsgSelectButton = 'fsel';


namespace BPrivate {


FileControl::FileControl(const char* name, const char* label,
	const char* pathOfFile, uint32 flavors)
	:
	BView(name, 0)
{
	SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
	SetLayout(new BGroupLayout(B_HORIZONTAL));

	fText = new BTextControl("file_path", label, pathOfFile, NULL);
	AddChild(fText);

	fButton = new BButton("select_file", B_TRANSLATE("Select" B_UTF8_ELLIPSIS),
		new BMessage(kMsgSelectButton));
	AddChild(fButton);

	fPanel = new BFilePanel(B_OPEN_PANEL, NULL, NULL, flavors, false);
}


FileControl::~FileControl()
{
	delete fPanel;
}


void
FileControl::AttachedToWindow()
{
	fButton->SetTarget(this);
	fPanel->SetTarget(this);
}


void
FileControl::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case kMsgSelectButton:
		{
			fPanel->Hide();

			BPath path(fText->Text());
			if (path.InitCheck() == B_OK && path.GetParent(&path) == B_OK)
				fPanel->SetPanelDirectory(path.Path());

			fPanel->Show();
			break;
		}
		case B_REFS_RECEIVED:
		{
			entry_ref ref;
			if (msg->FindRef("refs", &ref) == B_OK) {
				BEntry entry(&ref);
				if (entry.InitCheck() == B_OK) {
					BPath path;
					entry.GetPath(&path);

					fText->SetText(path.Path());
				}
			}
			break;
		}

		default:
			BView::MessageReceived(msg);
			break;
	}
}


void
FileControl::SetText(const char* pathOfFile)
{
	fText->SetText(pathOfFile);
}


const char*
FileControl::Text() const
{
	return fText->Text();
}


void
FileControl::SetEnabled(bool enabled)
{
	fText->SetEnabled(enabled);
	fButton->SetEnabled(enabled);
}


//	#pragma mark -


MailFileConfigView::MailFileConfigView(const char* label, const char* name,
	bool useMeta, const char* defaultPath, uint32 flavors)
	:
	FileControl(name, label, defaultPath, flavors),
	fUseMeta(useMeta),
	fName(name)
{
}


void
MailFileConfigView::SetTo(const BMessage* archive, BMessage* meta)
{
	SetText((fUseMeta ? meta : archive)->FindString(fName));
	fMeta = meta;
}


status_t
MailFileConfigView::SaveInto(BMailAddOnSettings& settings) const
{
	BMessage* archive = fUseMeta ? fMeta : &settings;
	return archive->SetString(fName, Text());
}


}	// namespace BPrivate
