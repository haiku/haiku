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


//******************************************************************************************************
//**** PROJECT HEADER FILES
//******************************************************************************************************
#include "BetterScrollView.h"
#include "Colors.h"


//******************************************************************************************************
//**** BetterScrollView CLASS
//******************************************************************************************************
#include <stdio.h>
BetterScrollView::BetterScrollView(const char *name, BView *target, uint32 resizeMask, uint32 flags,
	bool horizontal, bool vertical, bool scroll_view_corner, border_style border)
: BScrollView(name, target, resizeMask, flags, horizontal, vertical, border)
{
	m_target = target;
	m_data_rect.Set(-1,-1,-1,-1);
	m_h_scrollbar = ScrollBar(B_HORIZONTAL);
	m_v_scrollbar = ScrollBar(B_VERTICAL);
	if(scroll_view_corner && horizontal && vertical)
	{
		m_scroll_view_corner = new ScrollViewCorner(m_v_scrollbar->Frame().left,
			m_h_scrollbar->Frame().top);
		AddChild(m_scroll_view_corner);
	}
	else
		m_scroll_view_corner = NULL;
}


BetterScrollView::~BetterScrollView()
{ }


void BetterScrollView::SetDataRect(BRect data_rect, bool scrolling_allowed)
{
	m_data_rect = data_rect;
	UpdateScrollBars(scrolling_allowed);
}


void BetterScrollView::FrameResized(float new_width, float new_height)
{
	BScrollView::FrameResized(new_width,new_height);
	UpdateScrollBars(true);
}


void BetterScrollView::AttachedToWindow()
{
	BScrollView::AttachedToWindow();
	UpdateScrollBars(false);
}


void BetterScrollView::UpdateScrollBars(bool scrolling_allowed)
{
	//Figure out the bounds and scroll if necessary
	BRect view_bounds = m_target->Bounds();

	float page_width, page_height, view_width, view_height;
	view_width = view_bounds.right-view_bounds.left;
	view_height = view_bounds.bottom-view_bounds.top;

	float min,max;
	if(scrolling_allowed)
	{
		//Figure out the width of the page rectangle
		page_width = m_data_rect.right-m_data_rect.left;
		page_height = m_data_rect.bottom-m_data_rect.top;
		if(view_width > page_width)
			page_width = view_width;
		if(view_height > page_height)
			page_height = view_height;
	
		//Adjust positions
		float delta_x = 0.0;
		if(m_h_scrollbar)
		{
			if(view_bounds.left < m_data_rect.left)
				delta_x = m_data_rect.left - view_bounds.left;
			else if(view_bounds.right > m_data_rect.left+page_width)
				delta_x = m_data_rect.left+page_width - view_bounds.right;
		}
	
		float delta_y = 0.0;
		if(m_v_scrollbar)
		{
			if(view_bounds.top < m_data_rect.top)
				delta_y = m_data_rect.top - view_bounds.top;
			else if(view_bounds.bottom > m_data_rect.top+page_height)
				delta_y = m_data_rect.top+page_height - view_bounds.bottom;
		}
	
		if(delta_x != 0.0 || delta_y != 0.0)
		{
			m_target->ScrollTo(BPoint(view_bounds.left+delta_x,view_bounds.top+delta_y));
			view_bounds = Bounds();
		}
	}
	else
	{
		min = m_data_rect.left;
		if(view_bounds.left < min)
			min = view_bounds.left;
		max = m_data_rect.right;
		if(view_bounds.right > max)
			max = view_bounds.right;
		page_width = max-min;
		min = m_data_rect.top;
		if(view_bounds.top < min)
			min = view_bounds.top;
		max = m_data_rect.bottom;
		if(view_bounds.bottom > max)
			max = view_bounds.bottom;
		page_height = max-min;
	}

	//Figure out the ratio of the bounds rectangle width or height to the page rectangle width or height
	float width_prop = view_width/page_width;
	float height_prop = view_height/page_height;

	//Set the scroll bar ranges and proportions.  If the whole document is visible, inactivate the
	//slider
	bool active_scroller = false;
	if(m_h_scrollbar)
	{
		if(width_prop >= 1.0)
			m_h_scrollbar->SetRange(0.0,0.0);
		else
		{
			min = m_data_rect.left;
			max = m_data_rect.left + page_width - view_width;
			if(view_bounds.left < min)
				min = view_bounds.left;
			if(view_bounds.left > max)
				max = view_bounds.left;
			m_h_scrollbar->SetRange(min,max);
			m_h_scrollbar->SetSteps(ceil(view_width/20), view_width);
			active_scroller = true;
		}
		m_h_scrollbar->SetProportion(width_prop);
	}
	if(m_v_scrollbar)
	{
		if(height_prop >= 1.0)
			m_v_scrollbar->SetRange(0.0,0.0);
		else
		{
			min = m_data_rect.top;
			max = m_data_rect.top + page_height - view_height;
			if(view_bounds.top < min)
				min = view_bounds.top;
			if(view_bounds.top > max)
				max = view_bounds.top;
			m_v_scrollbar->SetRange(min,max);
			m_v_scrollbar->SetSteps(ceil(view_height/20), view_height);
			active_scroller = true;
		}
		m_v_scrollbar->SetProportion(height_prop);
	}
	if(m_scroll_view_corner)
	{
		rgb_color cur_color = m_scroll_view_corner->ViewColor();
		rgb_color new_color;
		if(active_scroller)
			new_color = BeBackgroundGrey;
		else
			new_color = BeInactiveControlGrey;
		if(new_color.red != cur_color.red || new_color.green != cur_color.green ||
			new_color.blue != cur_color.blue || new_color.alpha != cur_color.alpha)
		{
			m_scroll_view_corner->SetViewColor(new_color);
			m_scroll_view_corner->Invalidate();
		}
	}
}

