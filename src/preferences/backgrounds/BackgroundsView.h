/*
 * Copyright 2002-2009, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jerome Duval (jerome.duval@free.fr)
 */
#ifndef BACKGROUNDS_VIEW_H
#define BACKGROUNDS_VIEW_H


#include <View.h>
#include <ColorControl.h>
#include <Message.h>
#include <Button.h>
#include <ScrollView.h>
#include <ScrollBar.h>
#include <String.h>
#include <Box.h>
#include <CheckBox.h>
#include <TextControl.h>
#include <Menu.h>
#include <MenuItem.h>
#include <Entry.h>
#include <Screen.h>
#include <Control.h>
#include <Picture.h>
#include <FilePanel.h>
#include <StringView.h>
#include <Cursor.h>

#include "BackgroundImage.h"


#define SETTINGS_FILE		"Backgrounds_settings"


class ImageFilePanel;


class BGImageMenuItem : public BMenuItem {
public:
							BGImageMenuItem(const char* label, int32 imageIndex,
								BMessage* message, char shortcut = 0,
								uint32 modifiers = 0);

			int32			ImageIndex() { return fImageIndex; }

private:
			int32			fImageIndex;
};


enum frame_parts {
	FRAME_TOP_LEFT = 0,
	FRAME_TOP,
	FRAME_TOP_RIGHT,
	FRAME_LEFT_SIDE,
	FRAME_RIGHT_SIDE,
	FRAME_BOTTOM_LEFT,
	FRAME_BOTTOM,
	FRAME_BOTTOM_RIGHT,
};


class FramePart : public BView {
public:
							FramePart(int32 part);

			void			Draw(BRect rect);
			void			SetDesktop(bool isDesktop);

private:
			void			_SetSizeAndAlignment();

			int32			fFramePart;
			bool			fIsDesktop;
};


class PreView : public BControl {
public:
							PreView();

			BPoint			fPoint;
			BRect			fImageBounds;

protected:
			void			MouseDown(BPoint point);
			void			MouseUp(BPoint point);
			void			MouseMoved(BPoint point, uint32 transit,
								const BMessage* message);
			void			AttachedToWindow();

			BPoint			fOldPoint;
			float			x_ratio;
			float			y_ratio;
			display_mode	mode;

			BCursor			fMoveHandCursor;
};


class BackgroundsView : public BBox {
public:
							BackgroundsView();
							~BackgroundsView();

			void			RefsReceived(BMessage* msg);

			void			SaveSettings();
			void			WorkspaceActivated(uint32 oldWorkspaces,
								bool active);
			int32			AddImage(BPath path);
			Image*			GetImage(int32 imageIndex);

			bool			FoundPositionSetting();

protected:
			void			Save();
			void			NotifyServer();
			void			LoadSettings();
			void			AllAttached();
			void			MessageReceived(BMessage* msg);
			void			LoadDesktopFolder();
			void			LoadDefaultFolder();
			void			LoadFolder(bool isDesktop);
			void			LoadRecentFolder(BPath path);
			void			UpdateWithCurrent();
			void			UpdatePreview();
			void			UpdateButtons();
			void			SetDesktop(bool isDesktop);
			int32			AddPath(BPath path);

	static	int32			NotifyThread(void* data);

			BGImageMenuItem*	FindImageItem(const int32 imageIndex);

			bool			AddItem(BGImageMenuItem* item);

			BackgroundImage::Mode	FindPlacementMode();

			BColorControl*	fPicker;
			BButton*		fApply;
			BButton*		fRevert;
			BCheckBox*		fIconLabelOutline;
			BMenu*			fPlacementMenu;
			BMenu*			fImageMenu;
			BMenu*			fWorkspaceMenu;
			BTextControl*	fXPlacementText;
			BTextControl*	fYPlacementText;
			PreView*		fPreView;
			BBox*			fPreview;
			BFilePanel*		fFolderPanel;
			ImageFilePanel*	fPanel;

			BackgroundImage*	fCurrent;

			BackgroundImage::BackgroundImageInfo*	fCurrentInfo;

			entry_ref		fCurrentRef;
			int32			fLastImageIndex;
			int32			fLastWorkspaceIndex;
			BMessage		fSettings;

			BObjectList<BPath>	fPathList;
			BObjectList<Image>	fImageList;

			FramePart*		fTopLeft;
			FramePart*		fTop;
			FramePart*		fTopRight;
			FramePart*		fLeft;
			FramePart*		fRight;
			FramePart*		fBottomLeft;
			FramePart*		fBottom;
			FramePart*		fBottomRight;

			bool			fFoundPositionSetting;
};

#endif	// BACKGROUNDS_VIEW_H

