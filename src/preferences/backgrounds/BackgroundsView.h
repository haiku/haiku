/*
 * Copyright 2002-2005, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jerome Duval (jerome.duval@free.fr)
 */
#ifndef BACKGROUNDS_VIEW_H
#define BACKGROUNDS_VIEW_H


#include "BackgroundImage.h"

#include <View.h>
#include <ColorControl.h>
#include <Message.h>
#include <ListItem.h>
#include <ListView.h>
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

#define SETTINGS_FILE					"Backgrounds_settings"

class ImageFilePanel;

class BGImageMenuItem : public BMenuItem {
	public:
		BGImageMenuItem(const char *label, int32 imageIndex, BMessage *message, 
			char shortcut = 0, uint32 modifiers = 0);

		int32 ImageIndex() { return fImageIndex; }

	private:
		int32 fImageIndex;
};


class PreviewBox : public BBox {
	public:
		PreviewBox(BRect frame, const char *name);

		void Draw(BRect rect);
		void SetDesktop(bool isDesktop);

	protected:
		bool fIsDesktop;
};


class PreView : public BControl {
	public:
		PreView(BRect frame, const char *name, int32 resize, int32 flags);

		BPoint fPoint;
		BRect fImageBounds;

	protected:
		void MouseDown(BPoint point);
		void MouseUp(BPoint point);
		void MouseMoved(BPoint point, uint32 transit, const BMessage *message);
		void AttachedToWindow();

		BPoint fOldPoint;
		float x_ratio,y_ratio;
		display_mode mode;

		BCursor fMoveHandCursor;
};


class BackgroundsView : public BBox {
	public:
		BackgroundsView(BRect frame, const char *name, int32 resize, int32 flags);
		~BackgroundsView(void);

		void SaveSettings();
		void WorkspaceActivated(uint32 oldWorkspaces, bool active);
		int32 AddImage(BPath path);
		Image* GetImage(int32 imageIndex);
		void ProcessRefs(entry_ref dir_ref, BMessage* msg);

	protected:
		void Save();
		void NotifyServer();
		void LoadSettings();
		void AllAttached();
		void MessageReceived(BMessage *msg);
		void LoadDesktopFolder();
		void LoadDefaultFolder();
		void LoadFolder(bool isDesktop);
		void LoadRecentFolder(BPath path);
		void UpdateWithCurrent();
		void UpdatePreview();
		void UpdateButtons();
		void RefsReceived(BMessage *msg);
		int32 AddPath(BPath path);

		static int32 NotifyThread(void *data);

		BGImageMenuItem *FindImageItem(const int32 imageIndex);
		bool AddItem(BGImageMenuItem *item);

		BackgroundImage::Mode FindPlacementMode();

		BColorControl *fPicker;			// color picker
		BButton *fApply, *fRevert;		// apply and revert buttons
		BCheckBox *fIconLabelBackground;	// label ckeckbox
		BMenu* fPlacementMenu, *fImageMenu, *fWorkspaceMenu;	// the three comboboxes
		BTextControl *fXPlacementText, *fYPlacementText;		// x and y textboxes
		PreView *fPreView;				// the view for previewing the result
		PreviewBox *fPreview;			// the box which draws a computer/folder
		BFilePanel *fFolderPanel;		// the file panels for folders
		ImageFilePanel *fPanel;			// the file panels for images

		BackgroundImage *fCurrent;		// the current BackgroundImage object
		BackgroundImage::BackgroundImageInfo *fCurrentInfo;//the current BackgroundImageInfo object
		entry_ref fCurrentRef;			// the entry for the node which holds current 
		int32 fLastImageIndex, fLastWorkspaceIndex;		// last indexes for cancel
		BMessage fSettings;				// settings loaded from settings directory

		BObjectList<BPath> fPathList;
		BObjectList<Image> fImageList;
};

#endif	// BACKGROUNDS_VIEW_H
