#include <MenuItem.h>
#include <Message.h>
#include <Mime.h>
#include <FileTypeView.h>

FileTypeView::FileTypeView(BRect viewFrame)
	: BBox(viewFrame, "FileTypeView", B_FOLLOW_ALL,
	        B_FRAME_EVENTS|B_WILL_DRAW, B_PLAIN_BORDER)
{
	SetViewColor( ui_color(B_PANEL_BACKGROUND_COLOR) );
	
	const char * fileTypeLabel = "File Type";
	BRect fileTypeRect(10,10,viewFrame.Width()-55,90);
	fFileTypeBox = new BBox(fileTypeRect,fileTypeLabel,
	                        B_FOLLOW_LEFT_RIGHT|B_FOLLOW_TOP);
	fFileTypeBox->SetLabel(fileTypeLabel);
	AddChild(fFileTypeBox);
	
	BRect fileTypeTextControlRect(10,18,fileTypeRect.Width()-10,fileTypeRect.Height());
	fFileTypeTextControl = new BTextControl(fileTypeTextControlRect,"mime",
	                                        NULL,NULL,NULL,
	                                        B_FOLLOW_LEFT_RIGHT|B_FOLLOW_TOP);
	fFileTypeBox->AddChild(fFileTypeTextControl);
	
	const char * preferredAppLabel = "Preferred Application";
	BRect preferredAppRect(10,95,viewFrame.Width()-55,170);
	fPreferredAppBox = new BBox(preferredAppRect,preferredAppLabel,
	                            B_FOLLOW_LEFT_RIGHT|B_FOLLOW_BOTTOM);
	fPreferredAppBox->SetLabel(preferredAppLabel);
	AddChild(fPreferredAppBox);
	
	fPreferredAppMenu = new BMenu("app");
	fPreferredAppMenuItemNone = new BMenuItem("None",NULL);
	fPreferredAppMenu->AddItem(fPreferredAppMenuItemNone);
	fPreferredAppMenu->AddSeparatorItem();
	fPreferredAppMenu->SetRadioMode(true);
	fPreferredAppMenu->SetLabelFromMarked(true);
	fPreferredAppMenuItemNone->SetMarked(true);
	
	BRect preferredAppMenuFieldRect(10,15,preferredAppRect.Width()-10,
	                                preferredAppRect.Height());
	fPreferredAppMenuField = new BMenuField(preferredAppMenuFieldRect,
	                                        "appField",NULL,fPreferredAppMenu);
	fPreferredAppBox->AddChild(fPreferredAppMenuField);
}

FileTypeView::~FileTypeView()
{
}

void
FileTypeView::SetFileType(const char * fileType)
{
	fFileType.SetTo(fileType);
	fFileTypeTextControl->SetText(fileType);
	BMimeType mime(fileType);
	if (mime.InitCheck() != B_OK) {
		return;
	}
	BMessage applications;
	if (mime.GetSupportingApps(&applications) != B_OK) {
		return;
	}
	int32 subs = 0;
	if (applications.FindInt32("be:sub", &subs) != B_OK) {
		subs = 0;
	}
	int32 supers = 0;
	if (applications.FindInt32("be:super", &supers) != B_OK) {
		supers = 0;
	}
	for (int i = fPreferredAppMenu->CountItems() ; (i > 2) ; i--) {
		BMenuItem * item = fPreferredAppMenu->ItemAt(i);
		fPreferredAppMenu->RemoveItem(i);
		delete item;
	}
	bool separator = false;
	for (int i = 0 ; (i < subs+supers) ; i++) {
		const char * str;
		if (applications.FindString("applications", i, &str) == B_OK) {
			BMimeType app_mime(str);
			BMessage * message = NULL;
			entry_ref ref;
			if (app_mime.InitCheck() == B_OK) {
				if (app_mime.GetAppHint(&ref) == B_OK) {
					message = new BMessage();
					message->AddString("mimetype",str);
					str = ref.name;
				}
			}
			if (i < subs) {
				separator = true;
			} else if (separator) {
				fPreferredAppMenu->AddSeparatorItem();
				separator = false;
			}
			fPreferredAppMenu->AddItem(new BMenuItem(str,message));
		}
	}
}

void
FileTypeView::SetPreferredApplication(const char * preferredApplication)
{
	if (preferredApplication == 0) {
		fPreferredApp.SetTo(preferredApplication);
		fPreferredAppMenuItemNone->SetMarked(true);
	} else {
		BMenuItem * item = fPreferredAppMenu->FindItem(preferredApplication);
		if (item != 0) {
			fPreferredApp.SetTo(preferredApplication);
			item->SetMarked(true);
		}
	}
}

bool
FileTypeView::IsClean() const
{
	if (fFileType.Compare(GetFileType()) != 0) {
		return false;
	}
	if (fPreferredApp.Compare(GetPreferredApplication()) != 0) {
		return false;
	}
	return true;
}

const char *
FileTypeView::GetFileType() const
{
	return fFileTypeTextControl->Text();
}

const char *
FileTypeView::GetPreferredApplication() const
{
	BMenuItem * item = fPreferredAppMenu->FindMarked();
	if (item == 0) {
		return 0;
	}
	if (item == fPreferredAppMenuItemNone) {
		return 0;
	}
	return item->Label();
}
