#include <MenuItem.h>
#include <Message.h>
#include <Mime.h>
#include <Window.h>
#include <FileTypeView.h>

FileTypeView::FileTypeView(BRect viewFrame)
	: BBox(viewFrame, "FileTypeView", B_FOLLOW_ALL,
	        B_FRAME_EVENTS|B_WILL_DRAW, B_PLAIN_BORDER)
{
	SetViewColor( ui_color(B_PANEL_BACKGROUND_COLOR) );
	
	const char * fileTypeLabel = "File Type";
	BRect fileTypeRect(10,10,viewFrame.Width()-55,90);
	fFileTypeBox = new BBox(fileTypeRect,fileTypeLabel,
	                        B_FOLLOW_LEFT_RIGHT|B_FOLLOW_TOP, B_WILL_DRAW);
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
	                            B_FOLLOW_LEFT_RIGHT|B_FOLLOW_BOTTOM, B_WILL_DRAW);
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



class BAppMenuItem : public BMenuItem {
private:
	const char * fMimestr;
	BAppMenuItem(const char * label)
	 : BMenuItem(label,NULL) {
		fMimestr = 0;
	}
	void SetMime(const char * mimestr) {
		fMimestr = mimestr;
	}
	~BAppMenuItem() {
		if (fMimestr != Label()) {
			delete fMimestr;
		}
	}
public:
	const char * Mime() {
		return fMimestr;
	}
static BAppMenuItem * CreateItemForMime(const char * mimestr) {
		BMimeType mime(mimestr);
		entry_ref ref;
		const char * label = mimestr;
		if (mime.InitCheck() == B_OK) {
			if (mime.GetAppHint(&ref) == B_OK) {
				label = ref.name;
			}
		}
		BAppMenuItem * item = new BAppMenuItem(label);
		item->SetMime(strdup(mimestr));
		return item;
	}
};

void
FileTypeView::SetFileType(const char * fileType)
{
	bool fast = (fFileType.Compare(fileType) == 0);
	fFileType.SetTo(fileType);
	fFileTypeTextControl->SetText(fileType);
	if (fast)
		return;
	BWindow * window = Window();
	if (window) {
		window->DisableUpdates();
	}
	for (int i = fPreferredAppMenu->CountItems() ; (i > 1) ; i--) {
		BMenuItem * item = fPreferredAppMenu->ItemAt(i);
		fPreferredAppMenu->RemoveItem(i);
		delete item;
	}
	BMimeType mime(fileType);
	BMessage applications;
	if (mime.InitCheck() == B_OK) {
		if (mime.GetSupportingApps(&applications) != B_OK) {
			BMimeType super;
			if (mime.GetSupertype(&super) == B_OK) {
				if (super.GetSupportingApps(&applications) != B_OK) {
					applications.MakeEmpty();
				}
			}
		}
	}
	int32 subs = 0;
	if (applications.FindInt32("be:sub", &subs) != B_OK) {
		subs = 0;
	}
	int32 supers = 0;
	if (applications.FindInt32("be:super", &supers) != B_OK) {
		supers = 0;
	}
	bool separator = false;
	for (int i = 0 ; (i < subs+supers) ; i++) {
		const char * str;
		if (applications.FindString("applications", i, &str) == B_OK) {
			if (i < subs) {
				separator = true;
			} else if (separator) {
				fPreferredAppMenu->AddSeparatorItem();
				separator = false;
			}
			fPreferredAppMenu->AddItem(BAppMenuItem::CreateItemForMime(str));
		}
	}
	if (window) {
		window->EnableUpdates();
	}
}

void
FileTypeView::SetPreferredApplication(const char * preferredApplication)
{
	fPreferredApp.SetTo(preferredApplication);
	if ((preferredApplication == NULL) ||
        (strlen(preferredApplication) == 0)) {
		fPreferredAppMenuItemNone->SetMarked(true);
	} else {
		for (int i = 0 ; (i < fPreferredAppMenu->CountItems()) ; i++) {
			BAppMenuItem * item
			  = dynamic_cast<BAppMenuItem*>(fPreferredAppMenu->ItemAt(i));
			if (item) {
				if ((strcmp(item->Label(),preferredApplication) == 0) ||
					(strcmp(item->Mime(),preferredApplication) == 0)) {
					if (!item->IsMarked()) {
						item->SetMarked(true);
					}
					return;
				}
			}
		}
		BAppMenuItem * item = BAppMenuItem::CreateItemForMime(preferredApplication);
		fPreferredAppMenu->AddItem(item);
		item->SetMarked(true);
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
	BAppMenuItem * item
	  = dynamic_cast<BAppMenuItem*>(fPreferredAppMenu->FindMarked());
	if (item == 0) {
		return 0;
	}
	if (item == fPreferredAppMenuItemNone) {
		return 0;
	}
	return item->Mime();
}
