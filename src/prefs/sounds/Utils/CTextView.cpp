//*** LICENSE ***
//ColumnListView, its associated classes and source code, and the other components of Santa's Gift Bag are
//being made publicly available and free to use in freeware and shareware products with a price under $25
//(I believe that shareware should be cheap). For overpriced shareware (hehehe) or commercial products,
//please contact me to negotiate a fee for use. After all, I did work hard on this class and invested a lot
//of time into it. That being said, DON'T WORRY I don't want much. It totally depends on the sort of project
//you're working on and how much you expect to make off it. If someone makes money off my work, I'd like to
//get at least a little something.  If any of the components of Santa's Gift Bag are is used in a shareware
//or commercial product, I get a free copy.  The source is made available so that you can improve and extend
//it as you need. In general it is best to customize your ColumnListView through inheritance, so that you
//can take advantage of enhancements and bug fixes as they become available. Feel free to distribute the 
//ColumnListView source, including modified versions, but keep this documentation and license with it.


#include "CTextView.h"

CTextView::CTextView(BRect a_frame,const char* a_name,int32 a_resize_mode,int32 a_flags)
: BTextView(a_frame, a_name, BRect(4.0,4.0,a_frame.right-a_frame.left-4.0,a_frame.bottom-a_frame.top-4.0),
	a_resize_mode,a_flags)
{
	ResetTextRect();
	m_modified = false;
	m_modified_disabled = false;
}	


CTextView::~CTextView()
{ }


void CTextView::DetachedFromWindow()
{
	//This is sort of what the destructor should do, but...  Derived class's destructors get called before
	//CTextView's destructor so StoreChange won't get to the derived class's version of it.
	if(m_modified)
	{
		StoreChange();
		m_modified = false;
	}
}


void CTextView::FrameResized(float a_width, float a_height)
{
	ResetTextRect();
	BTextView::FrameResized(a_width,a_height);
}


void CTextView::MakeFocus(bool a_focused)
{
	BTextView::MakeFocus(a_focused);
	if(!a_focused && m_modified)
	{
		StoreChange();
		m_modified = false;
	}
}


void CTextView::InsertText(const char *a_text, int32 a_length, int32 a_offset,
	const text_run_array *a_runs)
{
	BTextView::InsertText(a_text, a_length, a_offset, a_runs);
	if(!m_modified_disabled)
		Modified();
}


void CTextView::Modified()
{
	m_modified = true;
}


void CTextView::SetText(const char *text, int32 length, const text_run_array *runs)
{
	m_modified_disabled = true;
	BTextView::SetText(text,length,runs);
	m_modified_disabled = false;
}


void CTextView::SetText(const char *text, const text_run_array *runs)
{
	m_modified_disabled = true;
	BTextView::SetText(text,runs);
	m_modified_disabled = false;
}


void CTextView::SetText(BFile *file, int32 offset, int32 length, const text_run_array *runs)
{
	m_modified_disabled = true;
	BTextView::SetText(file,offset,length,runs);
	m_modified_disabled = false;
}


void CTextView::StoreChange()
{ }


void CTextView::ResetTextRect()
{
	BRect textRect = Bounds();
	textRect.left = 4.0;
	textRect.top = 4.0;
	textRect.right -= 4.0;
	textRect.bottom -= 4.0;
	SetTextRect(textRect);
}


bool CTextView::HasBeenModified()
{
	return m_modified;
}
