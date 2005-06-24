/*

"Backgrounds" by Jerome Duval (jerome.duval@free.fr)

(C)2002 OpenBeOS under MIT license

*/

#ifndef BG_VIEW_H_
#define BG_VIEW_H_

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
#include "BackgroundImage.h"

#define SETTINGS_FILE					"Backgrounds_settings"

class BGImageMenuItem : public BMenuItem {
	public:
		BGImageMenuItem(const char *label, int32 imageIndex, BMessage *message, 
			char shortcut=0, uint32 modifiers=0);
		int32 ImageIndex() { return fImageIndex; }
	private:
		int32 fImageIndex;
};


class CustomRefFilter : public BRefFilter {
	public:
		CustomRefFilter(bool imageFiltering);
		bool Filter(const entry_ref *ref, BNode* node, struct stat *st, 
			const char *filetype);
	protected:
		bool fImageFiltering;	// true for images only, false for directory only
};


class ImageFilePanel : public BFilePanel {
	public:
		ImageFilePanel(file_panel_mode mode = B_OPEN_PANEL,
			BMessenger *target = 0, const entry_ref *start_directory = 0,
			uint32 node_flavors = 0, bool allow_multiple_selection = true,
			BMessage *message = 0, BRefFilter * = 0,
			bool modal = false, bool hide_when_done = true);
		virtual void SelectionChanged(void);
		void Show(void);
	
	protected:
		BView *imageView;
		BStringView *resolutionView, *imageTypeView;	
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


class BGView : public BBox {
	public:
		BGView(BRect frame, const char *name, int32 resize, int32 flags);
		~BGView(void);
	
		void SaveSettings(void);
		void WorkspaceActivated(uint32 oldWorkspaces, bool active);
		int32 AddImage(BPath path);
		Image* GetImage(int32 imageIndex);
		void ProcessRefs(entry_ref dir_ref, BMessage* msg);

	protected:
		void Save(void);
		void NotifyServer(void);
		void LoadSettings(void);
		void AllAttached(void);
		void MessageReceived(BMessage *msg);
		void LoadDesktopFolder(void);
		void LoadDefaultFolder(void);
		void LoadFolder(bool isDesktop);
		void LoadRecentFolder(BPath path);
		void UpdateWithCurrent(void);
		void UpdatePreview(void);
		void UpdateButtons(void);
		void RefsReceived(BMessage *msg);
		int32 AddPath(BPath path);
	
		static int32 NotifyThread(void *data);
	
		BGImageMenuItem *FindImageItem(const int32 imageIndex);
		bool AddItem(BGImageMenuItem *item);
	
		BackgroundImage::Mode FindPlacementMode();
	
		BColorControl *fPicker;			// color picker
		BButton *fApply,*fRevert;			// apply and revert buttons
		BCheckBox *fIconLabelBackground;	// label ckeckbox
		BMenu* fPlacementMenu, *fImageMenu, *fWorkspaceMenu;	// the three comboboxes
		BTextControl *fXPlacementText, *fYPlacementText;		// x and y textboxes
		PreView *fPreView;				// the view for previewing the result
		PreviewBox *fPreview;			// the box which draws a computer/folder
		BFilePanel *fFolderPanel;		// the file panels for folders
		ImageFilePanel *fPanel;			// the file panels for images
		
		BackgroundImage *fCurrent;		// the current BackgroundImage object
		BackgroundImage::BackgroundImageInfo *fCurrentInfo;//the current BackgroundImageInfo object
		entry_ref fCurrent_ref;		// the entry for the node which holds current 
		int32 fLastImageIndex, fLastWorkspaceIndex;		// last indexes for cancel
		BMessage fSettings;				// settings loaded from settings directory
		
		BObjectList<BPath> fPathList;
		BObjectList<Image> fImageList;
};

#endif
