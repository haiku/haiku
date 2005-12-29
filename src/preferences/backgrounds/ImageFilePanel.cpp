/*
 * Copyright 2002-2005, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jerome Duval (jerome.duval@free.fr)
 */


#include "ImageFilePanel.h"

#include <Bitmap.h>
#include <NodeInfo.h>
#include <String.h>
#include <StringView.h>
#include <TranslationUtils.h>
#include <Window.h>


ImageFilePanel::ImageFilePanel(file_panel_mode mode, BMessenger *target,
	const entry_ref *startDirectory, uint32 nodeFlavors,
	bool allowMultipleSelection, BMessage* message, BRefFilter* filter,
	bool modal,	bool hideWhenDone)
	: BFilePanel(mode, target, startDirectory, nodeFlavors,
		allowMultipleSelection, message, filter, modal, hideWhenDone),
	fImageView(NULL),
	fResolutionView(NULL),
	fImageTypeView(NULL)
{
}


void
ImageFilePanel::Show()
{
	if (fImageView == NULL) {
		Window()->Lock();
		BView *background = Window()->ChildAt(0);
		uint32 poseViewResizingMode = 
			background->FindView("PoseView")->ResizingMode();
		uint32 countVwResizingMode = 
			background->FindView("CountVw")->ResizingMode();
		uint32 vScrollBarResizingMode = 
			background->FindView("VScrollBar")->ResizingMode();
		uint32 hScrollBarResizingMode = 
			background->FindView("HScrollBar")->ResizingMode();

		background->FindView("PoseView")->SetResizingMode(B_FOLLOW_LEFT | B_FOLLOW_TOP);
		background->FindView("CountVw")->SetResizingMode(B_FOLLOW_LEFT | B_FOLLOW_TOP);
		background->FindView("VScrollBar")->SetResizingMode(B_FOLLOW_LEFT | B_FOLLOW_TOP);
		background->FindView("HScrollBar")->SetResizingMode(B_FOLLOW_LEFT | B_FOLLOW_TOP);
		Window()->ResizeBy(0, 70);
		background->FindView("PoseView")->SetResizingMode(poseViewResizingMode);
		background->FindView("CountVw")->SetResizingMode(countVwResizingMode);
		background->FindView("VScrollBar")->SetResizingMode(vScrollBarResizingMode);
		background->FindView("HScrollBar")->SetResizingMode(hScrollBarResizingMode);

		BRect rect(background->Bounds().left + 15, background->Bounds().bottom - 94,
			background->Bounds().left + 122, background->Bounds().bottom - 15);
		fImageView = new BView(rect, "ImageView", B_FOLLOW_LEFT | B_FOLLOW_BOTTOM, 
			B_SUBPIXEL_PRECISE);
		fImageView->SetViewColor(background->ViewColor());
		background->AddChild(fImageView);

		rect = BRect(background->Bounds().left + 132, background->Bounds().bottom - 85,
			background->Bounds().left + 250, background->Bounds().bottom - 65);
		fResolutionView = new BStringView(rect, "ResolutionView", NULL, 
			B_FOLLOW_LEFT | B_FOLLOW_BOTTOM);
		background->AddChild(fResolutionView);

		rect.OffsetBy(0, -16);
		fImageTypeView = new BStringView(rect, "ImageTypeView", NULL, 
			B_FOLLOW_LEFT | B_FOLLOW_BOTTOM);
		background->AddChild(fImageTypeView);

		Window()->Unlock();
	}

	BFilePanel::Show();
}


void
ImageFilePanel::SelectionChanged()
{
	entry_ref ref;
	Rewind();

	if (GetNextSelectedRef(&ref) == B_OK) {
		BEntry entry(&ref);
		BNode node(&ref);
		fImageView->ClearViewBitmap();

		if (node.IsFile()) {
			BBitmap *bitmap = BTranslationUtils::GetBitmap(&ref);

			if (bitmap != NULL) {
				BRect dest(fImageView->Bounds());
				if (bitmap->Bounds().Width()>bitmap->Bounds().Height()) {
					dest.InsetBy(0, (dest.Height()+1 - 
						((bitmap->Bounds().Height()+1) / 
						(bitmap->Bounds().Width()+1) * (dest.Width()+1)))/2);
				} else {
					dest.InsetBy((dest.Width()+1 - 
						((bitmap->Bounds().Width()+1) / 
						(bitmap->Bounds().Height()+1) * (dest.Height()+1)))/2, 0);
				}
				fImageView->SetViewBitmap(bitmap, bitmap->Bounds(), dest, 
					B_FOLLOW_LEFT | B_FOLLOW_TOP, 0);

				BString resolution;
				resolution << "Resolution: " << (int)(bitmap->Bounds().Width() + 1) 
					<< "x" << (int)(bitmap->Bounds().Height() + 1);
				fResolutionView->SetText(resolution.String());
				delete bitmap;

				BNode node(&ref);
				BNodeInfo nodeInfo(&node);
				char fileType[256];
				if (nodeInfo.GetType(fileType) == B_OK) {
					// TODO: this is broken!
					if (strncmp(fileType, "image/jpeg", 10) == 0)
						fImageTypeView->SetText("JPEG Image");
					else
						fImageTypeView->SetText("");
				}
			}
		} else {
			fResolutionView->SetText("");
			fImageTypeView->SetText("");
		}
		fImageView->Invalidate();
		fResolutionView->Invalidate();
	}

	BFilePanel::SelectionChanged();
}


//	#pragma mark -


CustomRefFilter::CustomRefFilter(bool imageFiltering)
	:
	fImageFiltering(imageFiltering)
{
}


bool
CustomRefFilter::Filter(const entry_ref *ref, BNode* node,
	struct stat *stat, const char *filetype)
{
	if (!fImageFiltering)
		return node->IsDirectory();

	if (node->IsDirectory())
		return true;

	BMimeType imageType("image"), refType(filetype);
	if (imageType.Contains(&refType))
		return true;

	return false;
}

