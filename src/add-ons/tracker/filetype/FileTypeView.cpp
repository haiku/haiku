#include <MenuItem.h>

#include <FileTypeView.h>

FileTypeView::FileTypeView(BRect viewFrame)
	: BView(viewFrame, "FileTypeView", B_FOLLOW_ALL,
	        B_FRAME_EVENTS|B_WILL_DRAW)
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
