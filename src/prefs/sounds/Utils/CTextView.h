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

//*** DESCRIPTION ***
//CTextView class

//Extends BTextView functionality somewhat.  Automates setting up the TextRect and updating it during resizing
//so that re-wrapping is live.  Also provides a StoreChanges hook function that is called when the CTextView
//loses focus or is removed from the window or destroyed.  This is useful if the view is of an object that can
//exist without the view, so that this more persistent object can be kept up-to-date when the text is modified,
//without having to update the other object every time a character is entered.


#ifndef _CTEXT_VIEW_H_
#define _CTEXT_VIEW_H_


#include <TextView.h>


class CTextView : public BTextView
{
	public:
		CTextView(BRect a_frame,const char* a_name,int32 a_resize_mode,int32 a_flags);
		virtual ~CTextView();

		//BTextView overrides
		virtual void DetachedFromWindow();
		virtual void FrameResized(float a_width, float a_height);
		virtual void MakeFocus(bool a_focused);
		void SetText(const char *text, int32 length, const text_run_array *runs = NULL);
		void SetText(const char *text, const text_run_array *runs = NULL);
		void SetText(BFile *file, int32 offset, int32 length, const text_run_array *runs = NULL);
		virtual void StoreChange();
		virtual void Modified();
		bool HasBeenModified();
		void ResetTextRect();

	protected:
		virtual void InsertText(const char *a_text, int32 a_length, int32 a_offset, const text_run_array *a_runs);

	private:
		bool m_modified;
		bool m_modified_disabled;
};


#endif
