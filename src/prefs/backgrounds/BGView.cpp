/*

"Backgrounds" by Jerome Duval (jerome.duval@free.fr)

(C)2002 OpenBeOS under MIT license

*/

/*
	BGView.cpp
*/

#include "BGView.h"

#include <OS.h>
#include <MenuField.h>
#include <stdio.h>
#include <stdlib.h>
#include <StorageKit.h>
#include <Background.h>
#include <Window.h>
#include <Messenger.h>
#include <Bitmap.h>
#include <Point.h>
#include <TranslationKit.h>
#include <PopUpMenu.h>

#define APPLY_SETTINGS 'aply'
#define REVERT_SETTINGS 'rvrt'
#define DEFAULT_SETTINGS 'dflt'
#define TRY_SETTINGS 'trys'

#define ATTRIBUTE_CHOSEN 'atch'
#define UPDATE_COLOR 'upcl'
#define ALL_WORKSPACES 'alwk'
#define CURRENT_WORKSPACE 'crwk'
#define DEFAULT_FOLDER 'dffl'
#define OTHER_FOLDER 'otfl'
#define NONE_IMAGE 'noim'
#define OTHER_IMAGE 'otim'
#define IMAGE_SELECTED 'imsl'
#define FOLDER_SELECTED 'flsl'

#define CENTER_PLACEMENT 'cnpl'
#define MANUAL_PLACEMENT 'mnpl'
#define SCALE_PLACEMENT 'scpl'
#define TILE_PLACEMENT 'tlpl'
#define ICONLABEL_CHECKBOX 'ilcb'

#define XY_PLACEMENT 'xypl'
#define PREVIEW_PLACEMENT 'pvpl'

void SetRGBColor(rgb_color *col,uint8 r, uint8 g, uint8 b, uint8 a=255);

uint8 hand_cursor_data[68] =
{
		16,1,2,2,
        
        0,0,		// 0000000000000000
        7,128,		// 0000011110000000
        61,112,		// 0011110101110000
        37,40,		// 0010010100101000
        36,168,		// 0010010010101000
        18,148,		// 0001001010010100
        18,84,		// 0001001001010100
        9,42,		// 0000100100101010
        8,1,		// 0000100000000001
        60,1,		// 0011110000000001
        76,1,		// 0100110000000001
        66,1,		// 0100001000000001
        48,1,		// 0011000000000001
        12,1,		// 0000110000000001
        2,0,		// 0000001000000000
        1,0,		// 0000000100000000
        
        0,0,		// 0000000000000000
        7,128,		// 0000011110000000
        63,240,		// 0011111111110000
        63,248,		// 0011111111111000
        63,248,		// 0011111111111000
        31,252,		// 0001111111111100
        31,252,		// 0001111111111100
        15,254,		// 0000111111111110
        15,255,		// 0000111111111111
        63,255,		// 0011111111111111
        127,255,	// 0111111111111111
        127,255,	// 0111111111111111
        63,255,		// 0011111111111111
        15,255,		// 0000111111111111
        3,254,		// 0000001111111110
        1,248		// 0000000111111000
};

BGView::BGView(BRect frame, const char *name, int32 resize, int32 flags)
	:BBox(frame, name, resize, flags | B_WILL_DRAW | B_FRAME_EVENTS, B_PLAIN_BORDER),
		fCurrent(NULL),
		fCurrentInfo(NULL),
		fLastImageIndex(-1),
		fPathList(1, true),
		fImageList(1, true)
{
	// SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	
	/* the preview box */
	
	BFont font(be_plain_font);
	fPreview = new PreviewBox(BRect(10,16,160,180), "preview");
	fPreview->SetFont(&font);
	fPreview->SetLabel("Preview");
	AddChild(fPreview);
	
	BRect xyrect(15, 135, 70, 155);
	fXPlacementText = new BTextControl(xyrect, "xPlacementText", "X:", NULL, 
		new BMessage(XY_PLACEMENT));
	fXPlacementText->SetDivider(12.0);
	fXPlacementText->TextView()->SetMaxBytes(4);
	fPreview->AddChild(fXPlacementText);
	
	xyrect.OffsetBy(65, 0);
	fYPlacementText = new BTextControl(xyrect, "yPlacementText", "Y:", NULL, 
		new BMessage(XY_PLACEMENT));
	fYPlacementText->SetDivider(12.0);
	fYPlacementText->TextView()->SetMaxBytes(4);
	fPreview->AddChild(fYPlacementText);
		
	for(int i=0; i<256; i++)
		if(i<'0'||i>'9') {
			fXPlacementText->TextView()->DisallowChar(i);
			fYPlacementText->TextView()->DisallowChar(i);
		}
	
	fPreView = new PreView(BRect(15,25,135,115), "preView", 
		B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW | B_SUBPIXEL_PRECISE);
	fPreview->AddChild(fPreView);
	
	/* the right box */
	
	BBox *rightbox = new BBox(BRect(fPreview->Frame().right+10, 
		fPreview->Frame().top-6,this->Frame().right-10,fPreview->Frame().bottom),
		"rightbox");
	AddChild(rightbox);
	
	fPicker=new BColorControl(BPoint(10, 110),B_CELLS_32x8,5.0,"Picker", 
		new BMessage(UPDATE_COLOR));
	rightbox->AddChild(fPicker);
	
	BRect cvrect(80, 76, 280, 96);
	
	fIconLabelBackground = new BCheckBox(cvrect, "iconLabelBackground", 
		"Icon label background", new BMessage(ICONLABEL_CHECKBOX));
	rightbox->AddChild(fIconLabelBackground);
	
	BMenuItem *menuItem;
	fWorkspaceMenu = new BPopUpMenu("pick one");
	fWorkspaceMenu->AddItem(menuItem = new BMenuItem("All Workspaces", 
		new BMessage(ALL_WORKSPACES)));
	fWorkspaceMenu->AddItem(menuItem = new BMenuItem("Current Workspace", 
		new BMessage(CURRENT_WORKSPACE)));
	menuItem->SetMarked(true);
	fLastWorkspaceIndex = fWorkspaceMenu->IndexOf(fWorkspaceMenu->FindMarked());
	fWorkspaceMenu->AddSeparatorItem();
	fWorkspaceMenu->AddItem(menuItem = new BMenuItem("Default folder", 
		new BMessage(DEFAULT_FOLDER)));
	fWorkspaceMenu->AddItem(menuItem = new BMenuItem("Other folder...", 
		new BMessage(OTHER_FOLDER)));
	fWorkspaceMenu->AddSeparatorItem();
	
	fImageMenu = new BPopUpMenu("pick one");
	fImageMenu->AddItem(new BGImageMenuItem("None", -1, new BMessage(NONE_IMAGE)));
	fImageMenu->AddSeparatorItem();
	fImageMenu->AddSeparatorItem();
	fImageMenu->AddItem(new BMenuItem("Other...", new BMessage(OTHER_IMAGE)));
	
	fPlacementMenu = new BPopUpMenu("pick one");
	fPlacementMenu->AddItem(new BMenuItem("Manual", 
		new BMessage(MANUAL_PLACEMENT)));
	fPlacementMenu->AddItem(new BMenuItem("Center", 
		new BMessage(CENTER_PLACEMENT)));
	fPlacementMenu->AddItem(new BMenuItem("Scale to fit", 
		new BMessage(SCALE_PLACEMENT)));
	fPlacementMenu->AddItem(new BMenuItem("Tile", 
		new BMessage(TILE_PLACEMENT)));
	
	cvrect.OffsetBy(-70, -25);
	BMenuField *placementMenuField = new BMenuField(cvrect, "placementMenuField",
		"Placement:", fPlacementMenu);
	placementMenuField->SetDivider(70.0);
	placementMenuField->SetAlignment(B_ALIGN_RIGHT);
	rightbox->AddChild(placementMenuField);
		
	cvrect.OffsetBy(0, -25);
	BMenuField *imageMenuField = new BMenuField(cvrect, "imageMenuField", 
		"Image:", fImageMenu);
	imageMenuField->SetDivider(70.0);
	imageMenuField->SetAlignment(B_ALIGN_RIGHT);
	rightbox->AddChild(imageMenuField);
	
	cvrect.right -= 70;
	cvrect.bottom -= 2;
	BMenuField *workspaceMenuField = new BMenuField(cvrect, "workspaceMenuField", 
		NULL, fWorkspaceMenu, true);
	rightbox->SetLabel(workspaceMenuField);
		
	fRevert=new BButton(BRect(fPreview->Frame().left, fPreview->Frame().bottom+10,
		fPreview->Frame().left+80, this->Frame().bottom-10),"RevertButton",
		"Revert", new BMessage(REVERT_SETTINGS),B_FOLLOW_LEFT |B_FOLLOW_TOP,
		B_WILL_DRAW | B_NAVIGABLE);
	AddChild(fRevert);
	
	fApply=new BButton(BRect(rightbox->Frame().right-70, fPreview->Frame().bottom
		+10,rightbox->Frame().right,110),"ApplyButton","Apply",
		new BMessage(APPLY_SETTINGS),B_FOLLOW_LEFT |B_FOLLOW_TOP,
		B_WILL_DRAW | B_NAVIGABLE);
	AddChild(fApply);
		
}


BGView::~BGView(void)
{
	delete fPanel;
	delete fFolderPanel;
}


void
BGView::AllAttached(void)
{
	fPlacementMenu->SetTargetForItems(this);
	fImageMenu->SetTargetForItems(this);
	fWorkspaceMenu->SetTargetForItems(this);
	fXPlacementText->SetTarget(this);
	fYPlacementText->SetTarget(this);
	fIconLabelBackground->SetTarget(this);
	fPicker->SetTarget(this);
	fApply->SetTarget(this);
	fRevert->SetTarget(this);
	
	fPanel = new ImageFilePanel(B_OPEN_PANEL, new BMessenger(this), NULL, 
		B_FILE_NODE, false, NULL, new CustomRefFilter(true));
	fPanel->SetButtonLabel(B_DEFAULT_BUTTON, "Select");
		
	fFolderPanel = new BFilePanel(B_OPEN_PANEL, new BMessenger(this), NULL, 
		B_DIRECTORY_NODE, false, NULL, new CustomRefFilter(false));
	fFolderPanel->SetButtonLabel(B_DEFAULT_BUTTON, "Select");
			
	LoadSettings();
	LoadDesktopFolder();
}


void
BGView::MessageReceived(BMessage *msg)
{
	//printf("what : %li\n", msg->what);
	switch(msg->what)
	{
		case B_SIMPLE_DATA:
		case B_REFS_RECEIVED:
		{
			//printf("refs received\n");
			RefsReceived(msg);
			break;
		}
		case PREVIEW_PLACEMENT:
		{
			BString xstring, ystring;
			xstring << (int)fPreView->fPoint.x;
			ystring << (int)fPreView->fPoint.y;
			fXPlacementText->SetText(xstring.String());
			fYPlacementText->SetText(ystring.String());
			UpdatePreview();
			UpdateButtons();
			break;
		}
		case MANUAL_PLACEMENT:
		{
			fXPlacementText->SetText("0");
			fYPlacementText->SetText("0");
			UpdatePreview();
			UpdateButtons();
			break;
		}
		case TILE_PLACEMENT:
		case SCALE_PLACEMENT:
		case CENTER_PLACEMENT:
		{
			fXPlacementText->SetText("");
			fYPlacementText->SetText("");
			UpdatePreview();
			UpdateButtons();
			break;
		}
		case ICONLABEL_CHECKBOX:
		{
			UpdateButtons();
			break;
		}
		case UPDATE_COLOR:
		case XY_PLACEMENT:
		{
			UpdatePreview();
			UpdateButtons();
			break;
		}
		case CURRENT_WORKSPACE:
		case ALL_WORKSPACES:
		{
			fImageMenu->FindItem(NONE_IMAGE)->SetLabel("None");
			fLastWorkspaceIndex = fWorkspaceMenu->IndexOf(
				fWorkspaceMenu->FindMarked());
			if(!fCurrent->IsDesktop()) {
				fPreview->SetDesktop(true);
				LoadDesktopFolder();
			} else
				UpdateButtons();
			break;
		}
		case DEFAULT_FOLDER:
		{
			fImageMenu->FindItem(NONE_IMAGE)->SetLabel("None");
			fLastWorkspaceIndex = fWorkspaceMenu->IndexOf(
				fWorkspaceMenu->FindMarked());
			fPreview->SetDesktop(false);
			LoadDefaultFolder();
			break;
		}
		case OTHER_FOLDER:
		{
			fFolderPanel->Show();
			break;
		}
		case OTHER_IMAGE:
		{
			fPanel->Show();
			break;
		}
		case B_CANCEL:
		{
			printf("cancel received\n");
			void* pointer;
			msg->FindPointer("source", &pointer);
			if(pointer == fPanel) {
				if(fLastImageIndex>=0)
					FindImageItem(fLastImageIndex)->SetMarked(true);
				else 
					fImageMenu->ItemAt(0)->SetMarked(true);
			}
			else if(pointer == fFolderPanel) {
				if(fLastWorkspaceIndex>=0)
					fWorkspaceMenu->ItemAt(fLastWorkspaceIndex)->SetMarked(true);
			}
			break;
		}
		case IMAGE_SELECTED:
		case NONE_IMAGE:
		{
			fLastImageIndex = ((BGImageMenuItem*)fImageMenu->FindMarked())
				->ImageIndex();
			UpdatePreview();
			UpdateButtons();
			break;
		}
		case FOLDER_SELECTED:
		{
			fImageMenu->FindItem(NONE_IMAGE)->SetLabel("Default");
			fLastWorkspaceIndex = fWorkspaceMenu->IndexOf(
				fWorkspaceMenu->FindMarked());
			fPreview->SetDesktop(false);
			LoadRecentFolder(*fPathList.ItemAt(fWorkspaceMenu->IndexOf(
				fWorkspaceMenu->FindMarked()) - 6));
			break;
		}
		case APPLY_SETTINGS:
		{
			Save();
			//NotifyServer();
			thread_id notify_thread;
			notify_thread = spawn_thread(BGView::NotifyThread, "notifyServer",
				B_NORMAL_PRIORITY, this);
			resume_thread(notify_thread);
			UpdateButtons();
			break;
		}
		case REVERT_SETTINGS:
		{
			UpdateWithCurrent();
			break;
		}
		default:
			BView::MessageReceived(msg);
			break;
	}
}


void
BGView::LoadDesktopFolder(void)
{
	BPath path;
	if(find_directory(B_DESKTOP_DIRECTORY, &path) == B_OK) {
		status_t err;
		err = get_ref_for_path(path.Path(), &fCurrent_ref);
		if(err!=B_OK)
			printf("error in LoadDesktopSettings\n");
		LoadFolder(true);
	}
}


void
BGView::LoadDefaultFolder(void)
{
	BPath path;
	if(find_directory(B_USER_SETTINGS_DIRECTORY, &path) == B_OK) {
		BString pathString = path.Path();
		pathString << "/Tracker/DefaultFolderTemplate";
		status_t err;
		err = get_ref_for_path(pathString.String(), &fCurrent_ref);
		if(err!=B_OK)
			printf("error in LoadDefaultFolderSettings\n");
		LoadFolder(false);
	}
}


void
BGView::LoadRecentFolder(BPath path)
{
	status_t err;
	err = get_ref_for_path(path.Path(), &fCurrent_ref);
	if(err!=B_OK)
		printf("error in LoadRecentFolder\n");
	LoadFolder(false);
}


void
BGView::LoadFolder(bool isDesktop)
{
	if(fCurrent) {
		delete fCurrent;
		fCurrent = NULL;
	}
	BNode node(&fCurrent_ref);
	if(node.InitCheck() == B_OK) {
		fCurrent = BackgroundImage::GetBackgroundImage(&node, isDesktop, this);
	} else {
		printf("pb with the folder node\n");
	}
	UpdateWithCurrent();
}


void
BGView::UpdateWithCurrent(void)
{
	if(!fCurrent)
		printf("current null\n");
	if(fCurrent) {
		fPlacementMenu->FindItem(SCALE_PLACEMENT)->SetEnabled(fCurrent->IsDesktop());
		fPlacementMenu->FindItem(CENTER_PLACEMENT)->SetEnabled(fCurrent->IsDesktop());
			
		if(fWorkspaceMenu->IndexOf(fWorkspaceMenu->FindMarked())>5)
			fImageMenu->FindItem(NONE_IMAGE)->SetLabel("Default");
		else
			fImageMenu->FindItem(NONE_IMAGE)->SetLabel("None");
		
		for(int32 i=fImageMenu->CountItems()-5; i>=0; i--)
			fImageMenu->RemoveItem(2);
	
		for(int32 i=fImageList.CountItems()-1; i>=0; i--) {
			BMessage *message = new BMessage(IMAGE_SELECTED);
			AddItem(new BGImageMenuItem(GetImage(i)->GetName(), i, message));
		}
		
		fImageMenu->SetTargetForItems(this);
				
		fCurrentInfo = fCurrent->ImageInfoForWorkspace(current_workspace());
			
		if(!fCurrentInfo) {
		
			fImageMenu->FindItem(NONE_IMAGE)->SetMarked(true);
			fPlacementMenu->FindItem(MANUAL_PLACEMENT)->SetMarked(true);
			
		} else {
			if(fCurrentInfo->fEraseTextWidgetBackground)
				fIconLabelBackground->SetValue(B_CONTROL_ON);
			else
				fIconLabelBackground->SetValue(B_CONTROL_OFF);
		
			BString xtext, ytext;
			int32 cmd = 0;
			switch(fCurrentInfo->fMode)
			{
				case BackgroundImage::kCentered:
					cmd = CENTER_PLACEMENT;
					break;
				case BackgroundImage::kScaledToFit:
					cmd = SCALE_PLACEMENT;
					break;
				case BackgroundImage::kAtOffset:
					cmd = MANUAL_PLACEMENT;
					xtext << (int)fCurrentInfo->fOffset.x;
					ytext << (int)fCurrentInfo->fOffset.y;
					break;	
				case BackgroundImage::kTiled:
					cmd = TILE_PLACEMENT;
				break;
			}
			if(cmd!=0)
				fPlacementMenu->FindItem(cmd)->SetMarked(true);
			fXPlacementText->SetText(xtext.String());
			fYPlacementText->SetText(ytext.String());
			
			fLastImageIndex = fCurrentInfo->fImageIndex;
			FindImageItem(fLastImageIndex)->SetMarked(true);
		}
		
		rgb_color color;
		if(fCurrent->IsDesktop()) {
			color = BScreen().DesktopColor();
			fPicker->SetEnabled(true);
		} else {
			SetRGBColor(&color,255,255,255);
			fPicker->SetEnabled(false);
		}
		fPicker->SetValue(color);
		
		UpdatePreview();
		UpdateButtons();
	}
}


void
BGView::Save()
{
	bool eraseTextWidgetBackground = 
		(fIconLabelBackground->Value()==B_CONTROL_ON);
	BackgroundImage::Mode mode = FindPlacementMode();
	BPoint offset(atoi(fXPlacementText->Text()), atoi(fYPlacementText->Text()));

	if(!fCurrent->IsDesktop()) {
		if(fCurrentInfo==NULL) {
			fCurrentInfo =  new BackgroundImage::BackgroundImageInfo(B_ALL_WORKSPACES, 
				fLastImageIndex, mode, offset, eraseTextWidgetBackground, 0, 0);
			fCurrent->Add(fCurrentInfo);
		} else {
			fCurrentInfo->fEraseTextWidgetBackground = eraseTextWidgetBackground;
			fCurrentInfo->fMode = mode;
			if(fCurrentInfo->fMode == BackgroundImage::kAtOffset)
				fCurrentInfo->fOffset = offset;
			fCurrentInfo->fImageIndex = fLastImageIndex;
		}		
	} else {
		uint32 workspaceMask = 1;
		uint32 workspace = current_workspace();
		for ( ; workspace; workspace--)
			workspaceMask *= 2;
	
		if(fCurrentInfo!=NULL) {
			if(fWorkspaceMenu->FindItem(CURRENT_WORKSPACE)->IsMarked()) {
			
				if(fCurrentInfo->fWorkspace != workspaceMask) {
					fCurrentInfo->fWorkspace = fCurrentInfo->fWorkspace 
						^ workspaceMask;
					fCurrentInfo = new BackgroundImage::BackgroundImageInfo(
						workspaceMask, fLastImageIndex, mode, offset, 
						eraseTextWidgetBackground, fCurrentInfo->fImageSet, 
						fCurrentInfo->fCacheMode);
					fCurrent->Add(fCurrentInfo);
				} else {
					fCurrentInfo->fEraseTextWidgetBackground = 
						eraseTextWidgetBackground;
					fCurrentInfo->fMode = mode;
					if(fCurrentInfo->fMode == BackgroundImage::kAtOffset) {
						fCurrentInfo->fOffset = offset;
					}
					fCurrentInfo->fImageIndex = fLastImageIndex;
				}
				
			} else {
				fCurrent->RemoveAll();
				fCurrentInfo =  new BackgroundImage::BackgroundImageInfo(
					B_ALL_WORKSPACES, fLastImageIndex, mode, offset, 
					eraseTextWidgetBackground, fCurrent->GetShowingImageSet(),
					fCurrentInfo->fCacheMode);
				fCurrent->Add(fCurrentInfo);
			}
		} else {
			if(fWorkspaceMenu->FindItem(CURRENT_WORKSPACE)->IsMarked()) {
				fCurrentInfo = new BackgroundImage::BackgroundImageInfo(
					workspaceMask, fLastImageIndex, mode, offset, 
					eraseTextWidgetBackground, fCurrent->GetShowingImageSet(), 0);
			} else {
				fCurrent->RemoveAll();
				fCurrentInfo =  new BackgroundImage::BackgroundImageInfo(
					B_ALL_WORKSPACES, fLastImageIndex, mode, offset, 
					eraseTextWidgetBackground, fCurrent->GetShowingImageSet(), 0);
			}
			fCurrent->Add(fCurrentInfo);
		}
		
		if(fCurrentInfo->fWorkspace == B_ALL_WORKSPACES)
			for(int32 i=0; i<count_workspaces(); i++)
				BScreen().SetDesktopColor(fPicker->ValueAsColor(), i, true);
		else
			BScreen().SetDesktopColor(fPicker->ValueAsColor(), true);
	}
	BNode node(&fCurrent_ref);
	fCurrent->SetBackgroundImage(&node);
}


void
BGView::NotifyServer()
{
	BMessenger tracker( "application/x-vnd.Be-TRAK" );
	if(fCurrent->IsDesktop()) {
		tracker.SendMessage( new BMessage(B_RESTORE_BACKGROUND_IMAGE));
	} else {
		int32 i = -1;
		BMessage reply; 
		int32 err;
		BEntry currentEnt( &fCurrent_ref );
		BPath currentPath( &currentEnt ); 
		bool isCustomFolder = !fWorkspaceMenu->FindItem(DEFAULT_FOLDER)->IsMarked();

		do { 
			i++;
		
    		/* scripting message */ 
    		BMessage msg( B_GET_PROPERTY ); 

    		/* look at the "Poses" in every Tracker window */ 
    		msg.AddSpecifier( "Poses" ); 
    		msg.AddSpecifier( "Window", i ); 

	        reply.MakeEmpty(); 

            tracker.SendMessage( &msg, &reply ); 

            /* break out of the loop when we're at the end of 
             * the windows 
             */ 
            if( reply.what == B_MESSAGE_NOT_UNDERSTOOD &&
                reply.FindInt32( "error", &err ) == B_OK &&
            err == B_BAD_INDEX ) 
            break; 

            /* don't stop for windows that don't understand 
             * a request for "Poses"; they're not displaying 
             * folders 
             */ 
            if( reply.what == B_MESSAGE_NOT_UNDERSTOOD &&
                reply.FindInt32( "error", &err ) == B_OK &&
                err != B_BAD_SCRIPT_SYNTAX ) 
                continue; 

            BMessenger m; 
            if( reply.FindMessenger( "result", &m ) != B_OK)
            	continue; 
            /* m is the messenger of a Tracker window with 
             * a folder inside 
             */ 

			if(isCustomFolder) {
           		/* found a Window with a Poses, ask for its Path */
           		msg.MakeEmpty(); 
           		msg.what = B_GET_PROPERTY; 
           		msg.AddSpecifier( "Path" ); 
           		msg.AddSpecifier( "Poses" ); 
           		msg.AddSpecifier( "Window", i ); 

           		reply.MakeEmpty();
           		
           		tracker.SendMessage( &msg, &reply ); 

           		/* go on with the next if this din't have a Path */ 
           		if( reply.what == B_MESSAGE_NOT_UNDERSTOOD ) continue; 
    
           		/* dig out the Path */ 
           		entry_ref ref; 
           		if( reply.FindRef( "result", &ref ) == B_OK ) {
           			BEntry ent( &ref ); 
            		BPath path( &ent ); 
        
            		/* these are not the Paths you're looking for */ 
            		if( currentPath != path) continue; 
           		}
        	}
        	
        	m.SendMessage( B_RESTORE_BACKGROUND_IMAGE );
		} while( true );
	}
}


int32 BGView::NotifyThread(void *data) {
	BGView *bgView = (BGView*)data;
	bgView->NotifyServer();
	return B_OK;
}


void
BGView::SaveSettings(void)
{
	BPath path;
	if(find_directory(B_USER_SETTINGS_DIRECTORY, &path) == B_OK) {
		path.Append(SETTINGS_FILE);
		BFile file(path.Path(),B_READ_WRITE|B_CREATE_FILE|B_ERASE_FILE);
	
		BPoint point = Window()->Frame().LeftTop();
		if(fSettings.ReplacePoint("pos", point)!=B_OK)
			fSettings.AddPoint("pos", point);
		
		entry_ref ref;
		BEntry entry;
		BPath path;
		
		fPanel->GetPanelDirectory(&ref);
		entry.SetTo(&ref);
		entry.GetPath(&path);
		if(fSettings.ReplaceString("paneldir", path.Path())!=B_OK)
			fSettings.AddString("paneldir", path.Path());
			
		fFolderPanel->GetPanelDirectory(&ref);
		entry.SetTo(&ref);
		entry.GetPath(&path);
		if(fSettings.ReplaceString("folderpaneldir", path.Path())!=B_OK)
			fSettings.AddString("folderpaneldir", path.Path());
			
		fSettings.RemoveName("recentfolder");
		for(int i=0; i<fPathList.CountItems();i++)
			fSettings.AddString("recentfolder", fPathList.ItemAt(i)->Path());
			
		fSettings.Flatten(&file);
	}
}


void
BGView::LoadSettings(void)
{
	fSettings.MakeEmpty();
	
	BPath path;
	if(find_directory(B_USER_SETTINGS_DIRECTORY, &path) == B_OK) {
		path.Append(SETTINGS_FILE);
		BFile file(path.Path(),B_READ_ONLY);
		if((file.InitCheck()==B_OK)&&(fSettings.Unflatten(&file)==B_OK))
		{
			fSettings.PrintToStream();
			
			BPoint point;
			if(fSettings.FindPoint("pos", &point)==B_OK)
				Window()->MoveTo(point);
			
			BString string;
			if(fSettings.FindString("paneldir", &string)==B_OK)
				fPanel->SetPanelDirectory(string.String());
			
			if(fSettings.FindString("folderpaneldir", &string)==B_OK)
				fFolderPanel->SetPanelDirectory(string.String());
			
			int32 index = 0;	
			while(fSettings.FindString("recentfolder", index, &string)==B_OK)
			{
				BPath path(string.String());
				int32 i = AddPath(path);
				BString s;
				s << "Folder: " << path.Leaf();
				BMenuItem *item = new BMenuItem(s.String(), 
					new BMessage(FOLDER_SELECTED));
				fWorkspaceMenu->AddItem(item, -i-1+6);
				index++;
			}
			fWorkspaceMenu->SetTargetForItems(this);
			
			printf("Settings Loaded\n");
		} else
		{
			printf("Error unflattening settings file %s\n",path.Path());
		}	
	}
}


void
SetRGBColor(rgb_color *col,uint8 r, uint8 g, uint8 b, uint8 a=255)
{
	col->red=r;
	col->green=g;
	col->blue=b;
	col->alpha=a;
}


void
BGView::WorkspaceActivated(uint32 oldWorkspaces, bool active)
{
	if(true) {
		UpdateWithCurrent();
	}
}


void
BGView::UpdatePreview()
{
	bool imageEnabled = !(fImageMenu->FindItem(NONE_IMAGE)->IsMarked());
	if(fPlacementMenu->IsEnabled()^imageEnabled)
		fPlacementMenu->SetEnabled(imageEnabled);
	if(fIconLabelBackground->IsEnabled()^imageEnabled)
		fIconLabelBackground->SetEnabled(imageEnabled);
	bool textEnabled = (fPlacementMenu->FindItem(MANUAL_PLACEMENT)->IsMarked())
		&&imageEnabled;
	if(fXPlacementText->IsEnabled()^textEnabled)
		fXPlacementText->SetEnabled(textEnabled);
	if(fYPlacementText->IsEnabled()^textEnabled)
		fYPlacementText->SetEnabled(textEnabled);
	fXPlacementText->TextView()->MakeSelectable(textEnabled);
	fYPlacementText->TextView()->MakeSelectable(textEnabled);
	fXPlacementText->TextView()->MakeEditable(textEnabled);
	fYPlacementText->TextView()->MakeEditable(textEnabled);
	int32 index = ((BGImageMenuItem*)fImageMenu->FindMarked())->ImageIndex();
	fPreView->ClearViewBitmap();
	if(index>=0) {
		BBitmap *bitmap = GetImage(index)->GetBitmap();
		if(bitmap) {
	
			BackgroundImage::BackgroundImageInfo *info = 
				new BackgroundImage::BackgroundImageInfo(0, index, 
					FindPlacementMode(), BPoint(atoi(fXPlacementText->Text()), 
					atoi(fYPlacementText->Text())), 
					(fIconLabelBackground->Value()==B_CONTROL_ON), 0, 0);
			if(info->fMode == BackgroundImage::kAtOffset) {
				fPreView->SetEnabled(true);
				fPreView->fPoint.x = atoi(fXPlacementText->Text());
				fPreView->fPoint.y = atoi(fYPlacementText->Text());
			} else {
				fPreView->SetEnabled(false);
			}	
	
			fPreView->fImageBounds = BRect(bitmap->Bounds());
			fCurrent->Show(info, fPreView);		
		}
	} else
		fPreView->SetEnabled(false);
	fPreView->SetViewColor(fPicker->ValueAsColor());
	fPreView->Invalidate();
}


BackgroundImage::Mode
BGView::FindPlacementMode()
{	
	BackgroundImage::Mode mode = BackgroundImage::kAtOffset;
	if(fPlacementMenu->FindItem(CENTER_PLACEMENT)->IsMarked())
		mode = BackgroundImage::kCentered;
	if(fPlacementMenu->FindItem(SCALE_PLACEMENT)->IsMarked())
		mode = BackgroundImage::kScaledToFit;
	if(fPlacementMenu->FindItem(MANUAL_PLACEMENT)->IsMarked())
		mode = BackgroundImage::kAtOffset;
	if(fPlacementMenu->FindItem(TILE_PLACEMENT)->IsMarked())
		mode = BackgroundImage::kTiled;
	return mode;
}


void
BGView::UpdateButtons()
{
	bool hasChanged = false;
	if(fPicker->IsEnabled() && ((fPicker->ValueAsColor().green != 
			BScreen().DesktopColor().green)
			||(fPicker->ValueAsColor().red != BScreen().DesktopColor().red)
			||(fPicker->ValueAsColor().blue != BScreen().DesktopColor().blue)))
			hasChanged = true;
	else if(fCurrentInfo) {
		if((fIconLabelBackground->Value()==B_CONTROL_ON) ^ 
			fCurrentInfo->fEraseTextWidgetBackground)
			hasChanged = true;
		else if(FindPlacementMode() != fCurrentInfo->fMode)
			hasChanged = true;
		else if(fCurrentInfo->fImageIndex != 
			((BGImageMenuItem*)fImageMenu->FindMarked())->ImageIndex())
			hasChanged = true;
		else if(fCurrent->IsDesktop()&&((fCurrentInfo->fWorkspace != B_ALL_WORKSPACES)
			 ^ (fWorkspaceMenu->FindItem(CURRENT_WORKSPACE)->IsMarked())))
			hasChanged = true;
		else if(fCurrentInfo->fMode == BackgroundImage::kAtOffset) {
				BString oldString, newString;
				oldString << (int)fCurrentInfo->fOffset.x;
				if(oldString != BString(fXPlacementText->Text()))
					hasChanged = true;
				oldString = "";
				oldString << (int)fCurrentInfo->fOffset.y;
				if(oldString != BString(fYPlacementText->Text()))
					hasChanged = true;
		}
	} else {
		if(fImageMenu->IndexOf(fImageMenu->FindMarked())>0)
			hasChanged = true;
	}

	fApply->SetEnabled(hasChanged);
	fRevert->SetEnabled(hasChanged);
}


void 
BGView::RefsReceived(BMessage *msg)
{
	entry_ref ref;
	int32 i = 0;
	BMimeType imageType("image");
	while (msg->FindRef("refs", i++, &ref) == B_OK) {
		
		BPath path;
		BEntry entry(&ref, true);
		path.SetTo(&entry);
		BNode node(&entry);
		
		if (node.IsFile()) {
			BNodeInfo nodeInfo(&node);
			char fileType[256];
			if(nodeInfo.GetType(fileType)!=B_OK)
				continue;
			BMimeType refType(fileType);
			if(!imageType.Contains(&refType))
				continue;
			BGImageMenuItem *item;
			int32 index = AddImage(path);
			if(index>=0) {
				item = FindImageItem(index);
				fLastImageIndex = index;
			} else {
				const char* name = GetImage(-index-1)->GetName();
				item = new BGImageMenuItem(name, -index-1, 
					new BMessage(IMAGE_SELECTED));
				AddItem(item);
				item->SetTarget(this);
				fLastImageIndex = -index-1;
			}
			item->SetMarked(true);
			BMessenger messenger(this);
			messenger.SendMessage(IMAGE_SELECTED);
		} else if(node.IsDirectory()) {
			BMenuItem *item;
			int32 index = AddPath(path);
			if(index>=0) {
				item = fWorkspaceMenu->ItemAt(index+6);
				fLastWorkspaceIndex = index+6;
			} else {
				BString s;
				s << "Folder: " << path.Leaf();
				item = new BMenuItem(s.String(), new BMessage(FOLDER_SELECTED));
				fWorkspaceMenu->AddItem(item, -index-1+6);
				item->SetTarget(this);
				fLastWorkspaceIndex = -index-1 + 6;
			}
			item->SetMarked(true);
			BMessenger messenger(this);
			messenger.SendMessage(FOLDER_SELECTED);
		}
	}
}


int32
BGView::AddPath(BPath path)
{
	int32 count = fPathList.CountItems();
	int32 index=0;
	for(; index<count ;index++) {
		BPath *p = fPathList.ItemAt(index);
		int c = (BString(p->Path()).ICompare(BString(path.Path())));
		if(c == 0)
			return index;
		else if(c > 0)
			break;
	}
	fPathList.AddItem(new BPath(path), index);
	return -index-1;	
}


int32
BGView::AddImage(BPath path)
{
	int32 count = fImageList.CountItems();
	int32 index=0;
	for(; index<count ;index++) {
		Image *image = fImageList.ItemAt(index);
		if(image->GetPath() == path)
			return index;
	}
	fImageList.AddItem(new Image(path));
		
	return -index-1;	
}


void
BGView::ProcessRefs(entry_ref dir_ref, BMessage* msg)
{
	fWorkspaceMenu->FindItem(DEFAULT_FOLDER)->SetMarked(true);
	BMessenger messenger(this);
	messenger.SendMessage(DEFAULT_FOLDER);
	if(msg->CountNames(B_REF_TYPE)>0) {
		messenger.SendMessage(msg);
	} else {
		BMessage message(B_REFS_RECEIVED);
		message.AddRef("refs", &dir_ref);
		messenger.SendMessage(&message);
	}
}


Image*
BGView::GetImage(int32 imageIndex)
{
	return fImageList.ItemAt(imageIndex);
}


BGImageMenuItem*
BGView::FindImageItem(const int32 imageIndex)
{
	if(imageIndex<0)
		return (BGImageMenuItem *)fImageMenu->ItemAt(0);
	int32 count = fImageMenu->CountItems() - 2;
	int32 index=2;
	for(; index<count ;index++) {
		BGImageMenuItem *image = (BGImageMenuItem *)fImageMenu->ItemAt(index);
		if(image->ImageIndex() == imageIndex)
			return image;
	}
	return NULL;
}


bool
BGView::AddItem(BGImageMenuItem *item)
{
	int32 count = fImageMenu->CountItems() - 2;
	int32 index=2;
	for(; index<count ;index++) {
		BGImageMenuItem *image = (BGImageMenuItem *)fImageMenu->ItemAt(index);
		int c = (BString(image->Label()).ICompare(BString(item->Label())));
		if(c > 0)
			break;
	}
	return fImageMenu->AddItem(item, index);
}


PreView::PreView(BRect frame, const char *name, int32 resize, int32 flags)
	:BControl(frame,name,NULL, NULL, resize,flags),
	fMoveHandCursor(hand_cursor_data)
{
	
}


void
PreView::AttachedToWindow() {
	rgb_color color = ViewColor();
	BControl::AttachedToWindow();
	SetViewColor(color);
}


void
PreView::MouseDown(BPoint point)
{
	if (IsEnabled() && Bounds().Contains(point)) {
		uint32 buttons;
		GetMouse(&point, &buttons);
	 	if(buttons & B_PRIMARY_MOUSE_BUTTON) {
			fOldPoint = point;
			SetTracking(true);
			BScreen().GetMode(&mode);
			x_ratio = Bounds().Width() / mode.virtual_width;
			y_ratio = Bounds().Height() / mode.virtual_height;
			SetMouseEventMask(B_POINTER_EVENTS,
				B_LOCK_WINDOW_FOCUS | B_NO_POINTER_HISTORY);
		}
	}
}


void
PreView::MouseUp(BPoint point)
{
	if(IsTracking())
		SetTracking(false);
}


void
PreView::MouseMoved(BPoint point, uint32 transit, const BMessage *message)
{
	if(IsEnabled()) 
		SetViewCursor(&fMoveHandCursor);
	else
		SetViewCursor(B_CURSOR_SYSTEM_DEFAULT);
	if(IsTracking())
	{
		float x, y;
		x = fPoint.x + (point.x - fOldPoint.x)/ x_ratio;
		y = fPoint.y + (point.y - fOldPoint.y)/ y_ratio;
		bool min, max, mustSend = false;
		min = (x > -fImageBounds.Width());
		max = (x < mode.virtual_width);
		if( min && max) {
			fOldPoint.x = point.x;
			fPoint.x = x;
			mustSend = true;
		} else {
			if(!min && fPoint.x > -fImageBounds.Width()) {
				fPoint.x = -fImageBounds.Width();
				fOldPoint.x = point.x - (x - fPoint.x) * x_ratio;
				mustSend = true;
			}
			if(!max && fPoint.x < mode.virtual_width) {
				fPoint.x = mode.virtual_width;
				fOldPoint.x = point.x - (x - fPoint.x) * x_ratio;
				mustSend = true;
			}
		}				
		
		min = (y > -fImageBounds.Height());
		max = (y < mode.virtual_height);
		if(min && max) {
			fOldPoint.y = point.y;
			fPoint.y = y;
			mustSend = true;
		} else {
			if(!min && fPoint.y > -fImageBounds.Height()) {
				fPoint.y = -fImageBounds.Height();
				fOldPoint.y = point.y - (y - fPoint.y) * y_ratio;
				mustSend = true;
			}
			if(!max && fPoint.y < mode.virtual_height) {
				fPoint.y = mode.virtual_height;
				fOldPoint.y = point.y - (y - fPoint.y) * y_ratio;
				mustSend = true;
			}
		}
		
		if(mustSend) {
			BMessenger messenger(Parent());
			messenger.SendMessage(PREVIEW_PLACEMENT);
		}
	}
	BControl::MouseMoved(point, transit, message);
}


PreviewBox::PreviewBox(BRect frame, const char *name)
	:BBox(frame,name)
{
	fIsDesktop = true;
}


void
PreviewBox::Draw(BRect rect)
{
	rgb_color color = HighColor();
	BPoint points[] = { BPoint(11,19), BPoint(139,19), BPoint(141,21), 
						BPoint(141,119), BPoint(139,121), BPoint(118,121), 
						BPoint(118,126), BPoint(117,127), BPoint(33,127),
						BPoint(32,126), BPoint(32,121),BPoint(11,121),
						BPoint(9,119),BPoint(9,21),BPoint(11,19) };
	
	SetHighColor(LowColor());
	FillRect(BRect(9,19,141,127));
	if(fIsDesktop) {				
		SetHighColor(184,184,184);
		FillPolygon(points, 15);
		SetHighColor(96,96,96);
		StrokePolygon(points, 15);
		FillRect(BRect(107,121,111,123));
		SetHighColor(0,0,0);
		StrokeRect(BRect(14,24,136,116));
		SetHighColor(0,255,0);
		FillRect(BRect(101,122,103,123));
	} else {
		SetHighColor(152,152,152);
		StrokeLine(BPoint(11,13), BPoint(67,13));
		StrokeLine(BPoint(67,21));
		StrokeLine(BPoint(139,21));
		StrokeLine(BPoint(139,119));
		StrokeLine(BPoint(11,119));
		StrokeLine(BPoint(11,13));
		StrokeRect(BRect(14,24,136,116));
		SetHighColor(255,203,0);
		FillRect(BRect(12,14,66,21));
		SetHighColor(240,240,240);
		StrokeRect(BRect(12,22,137,117));
		StrokeLine(BPoint(138,22), BPoint(138,22));
		StrokeLine(BPoint(12,118), BPoint(12,118));
		SetHighColor(200,200,200);
		StrokeRect(BRect(13,23,138,118));
	}
	SetHighColor(color);
	BBox::Draw(rect);
}


void
PreviewBox::SetDesktop(bool isDesktop)
{
	fIsDesktop = isDesktop;
	Invalidate();
}


ImageFilePanel::ImageFilePanel(file_panel_mode mode, BMessenger *target, 
	const entry_ref *start_directory, uint32 node_flavors, 
	bool allow_multiple_selection, BMessage *message, BRefFilter *filter,	
	bool modal,	bool hide_when_done)
	:BFilePanel(mode,target,start_directory, node_flavors, 
		allow_multiple_selection,message,filter,modal,hide_when_done),
	imageView(NULL)
{
}


void
ImageFilePanel::Show()
{
	if(imageView==NULL) {
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
				
		background->FindView("PoseView")->SetResizingMode(B_FOLLOW_LEFT|B_FOLLOW_TOP);
		background->FindView("CountVw")->SetResizingMode(B_FOLLOW_LEFT|B_FOLLOW_TOP);
		background->FindView("VScrollBar")->SetResizingMode(B_FOLLOW_LEFT|B_FOLLOW_TOP);
		background->FindView("HScrollBar")->SetResizingMode(B_FOLLOW_LEFT|B_FOLLOW_TOP);
		Window()->ResizeBy(0, 70);
		background->FindView("PoseView")->SetResizingMode(poseViewResizingMode);
		background->FindView("CountVw")->SetResizingMode(countVwResizingMode);
		background->FindView("VScrollBar")->SetResizingMode(vScrollBarResizingMode);
		background->FindView("HScrollBar")->SetResizingMode(hScrollBarResizingMode);
				
		BRect rect(background->Bounds().left+15,background->Bounds().bottom-94,
			background->Bounds().left+122,background->Bounds().bottom-15);
		imageView = new BView(rect, "ImageView", B_FOLLOW_LEFT|B_FOLLOW_BOTTOM, 
			B_SUBPIXEL_PRECISE);
		imageView->SetViewColor(background->ViewColor());
		background->AddChild(imageView);
				
		rect = BRect(background->Bounds().left+132,background->Bounds().bottom-85,
			background->Bounds().left+250,background->Bounds().bottom-65);
		resolutionView = new BStringView(rect, "ResolutionView", NULL, 
			B_FOLLOW_LEFT | B_FOLLOW_BOTTOM);
		background->AddChild(resolutionView);
				
		rect.OffsetBy(0, -16);
		imageTypeView = new BStringView(rect, "ImageTypeView", NULL, 
			B_FOLLOW_LEFT | B_FOLLOW_BOTTOM);
		background->AddChild(imageTypeView);
		Window()->Unlock();
	}
	BFilePanel::Show();
}


void
ImageFilePanel::SelectionChanged(void)
{
	entry_ref ref;
	Rewind();
	if(GetNextSelectedRef(&ref)==B_OK) {
		BEntry entry(&ref);
		BNode node(&ref);
		imageView->ClearViewBitmap();
		
		if(node.IsFile()) {
			BBitmap *bitmap = BTranslationUtils::GetBitmap(&ref);
						
			if(bitmap!=NULL) {
				BRect dest(imageView->Bounds());
				if(bitmap->Bounds().Width()>bitmap->Bounds().Height()) {
					dest.InsetBy(0, (dest.Height()+1 - 
						((bitmap->Bounds().Height()+1) / 
						(bitmap->Bounds().Width()+1) * (dest.Width()+1)))/2);
				} else {
					dest.InsetBy((dest.Width()+1 - 
					((bitmap->Bounds().Width()+1) / 
					(bitmap->Bounds().Height()+1) * (dest.Height()+1)))/2, 0);
				}
				imageView->SetViewBitmap(bitmap, bitmap->Bounds(), dest, 
					B_FOLLOW_LEFT | B_FOLLOW_TOP, 0);
				
				BString resolution;
				resolution << "Resolution: " << (int)(bitmap->Bounds().Width()+1) 
					<< "x" << (int)(bitmap->Bounds().Height()+1);
				resolutionView->SetText(resolution.String());
				delete bitmap;
				
				BNode node(&ref);
				BNodeInfo nodeInfo(&node);
				char fileType[256];
				if(nodeInfo.GetType(fileType)==B_OK)
				{
					if(strncmp(fileType, "image/jpeg", 10)==0)
						imageTypeView->SetText("JPEG Image");
					else
						imageTypeView->SetText("");	
				}
			}
		} else {
			resolutionView->SetText("");
			imageTypeView->SetText("");
		}
		imageView->Invalidate();
		resolutionView->Invalidate();
	}
	BFilePanel::SelectionChanged();
}


CustomRefFilter::CustomRefFilter(bool imageFiltering)
	: BRefFilter(),
		fImageFiltering(imageFiltering)
{
}


bool
CustomRefFilter::Filter(const entry_ref *ref, BNode* node, struct stat *st, 
	const char *filetype)
{
	if(fImageFiltering){
		if(node->IsDirectory())
			return true;
		else {
			BMimeType imageType("image"), refType(filetype);
			if(imageType.Contains(&refType))
				return true;
		}
		return false;
	} else {
		return (node->IsDirectory());
	}
}


BGImageMenuItem::BGImageMenuItem(const char *label, int32 imageIndex, 
	BMessage *message, char shortcut, uint32 modifiers)
	:BMenuItem(label, message, shortcut, modifiers),
	fImageIndex(imageIndex)
{
}
