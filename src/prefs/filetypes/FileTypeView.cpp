#include <FileTypeView.h>

FileTypeView::FileTypeView(BRect viewFrame)
	: BView(viewFrame, "FileTypeView", B_FOLLOW_ALL,
	        B_FRAME_EVENTS|B_WILL_DRAW)
{
	fFileTypeBox = new BBox(BRect(25,25,325,175),"File Type");
	AddChild(fFileTypeBox);
	
	fPreferredApplicationBox = new BBox(BRect(25,225,325,375),"Preferred Application");
	AddChild(fPreferredApplicationBox);
}

FileTypeView::~FileTypeView()
{
}

void
FileTypeView::SetFileType(const char * fileType)
{
	fFileType.SetTo(fileType);
	// change UI
}

void
FileTypeView::SetPreferredApplication(const char * preferredApplication)
{
	fPreferredApplication.SetTo(preferredApplication);
	// change UI
}

bool
FileTypeView::Clean()
{
	if (fFileType.Compare(GetFileType()) != 0) {
		return false;
	}
	if (fPreferredApplication.Compare(GetPreferredApplication()) != 0) {
		return false;
	}
	return true;
}

const char *
FileTypeView::GetFileType()
{
	// return from UI
	return "";
}

const char *
FileTypeView::GetPreferredApplication()
{
	// return from UI
	return "";
}
