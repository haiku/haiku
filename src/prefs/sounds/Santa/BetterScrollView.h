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

//Conventions:
//    Global constants (declared with const) and #defines - all uppercase letters with words separated 
//        by underscores.
//        (E.G., #define MY_DEFINE 5).
//        (E.G., const int MY_CONSTANT = 5;).
//    New data types (classes, structs, typedefs, etc.) - begin with an uppercase letter followed by
//        lowercase words separated by uppercase letters.  Enumerated constants contain a prefix
//        associating them with a particular enumerated set.
//        (E.G., typedef int MyTypedef;).
//        (E.G., enum MyEnumConst {MEC_ONE, MEC_TWO};)
//    Global variables - begin with "g_" followed by lowercase words separated by underscores.
//        (E.G., int g_my_global;).
//    Argument and local variables - begin with a lowercase letter followed by
//        lowercase words separated by underscores.
//        (E.G., int my_local;).
//    Member variables - begin with "m_" followed by lowercase words separated by underscores.
//        (E.G., int m_my_member;).
//    Functions (member or global) - begin with an uppercase letter followed by lowercase words
//        separated by uppercase letters.
//        (E.G., void MyFunction(void);).


#ifndef _SGB_BETTER_SCROLL_VIEW_H_
#define _SGB_BETTER_SCROLL_VIEW_H_


//******************************************************************************************************
//**** PROJECT HEADER FILES
//******************************************************************************************************
#include <ScrollView.h>


//******************************************************************************************************
//**** PROJECT HEADER FILES
//******************************************************************************************************
#include "ScrollViewCorner.h"


//******************************************************************************************************
//**** CLASS DECLARATIONS
//******************************************************************************************************
class BetterScrollView : public BScrollView
{
	public:
		BetterScrollView(const char *name, BView *target, uint32 resizeMask = B_FOLLOW_LEFT | B_FOLLOW_TOP,
			uint32 flags = B_FRAME_EVENTS | B_WILL_DRAW, bool horizontal = true, bool vertical = true,
			bool scroll_view_corner = true, border_style border = B_FANCY_BORDER);
		virtual ~BetterScrollView();
		virtual void SetDataRect(BRect data_rect, bool scrolling_allowed = true);
		inline BRect DataRect() {return m_data_rect;}
		virtual	void FrameResized(float new_width, float new_height);
		virtual void AttachedToWindow();

	private:
		void UpdateScrollBars(bool scrolling_allowed);

		BRect m_data_rect;
		BScrollBar* m_h_scrollbar;
		BScrollBar* m_v_scrollbar;
		ScrollViewCorner* m_scroll_view_corner;
		BView* m_target;
};


#endif
