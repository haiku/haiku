//Name:		TextEntryAlert.cpp
//Author:	Brian Tietz
//Conventions:
//	Global constants (declared with const) and #defines - begin with "c_" followed by lowercase
//		words separated by underscores.
//		(E.G., #define c_my_constant 5).
//		(E.G., const int c_my_constant = 5;).
//	Global variables - begin with "g_" followed by lowercase words separated by underscores.
//		(E.G., int g_my_global;).
//	New data types (classes, structs, typedefs, etc.) - begin with an uppercase letter followed by
//		lowercase words separated by uppercase letters.  Enumerated constants contain a prefix
//		associating them with a particular enumerated set.
//		(E.G., typedef int MyTypedef;).
//		(E.G., enum MyEnumConst {c_mec_one, c_mec_two};)
//	Private member variables - begin with "m_" followed by lowercase words separated by underscores.
//		(E.G., int m_my_member;).
//	Public or friend-accessible member variables - all lowercase words separated by underscores.
//		(E.G., int public_member;).
//	Argument and local variables - begin with a lowercase letter followed by
//		lowercase words separated by underscores.  If the name is already taken by a public member
//		variable, prefix with a_ or l_
//		(E.G., int my_local; int a_my_arg, int l_my_local).
//	Functions (member or global) - begin with an uppercase letter followed by lowercase words
//		separated by uppercase letters.
//		(E.G., void MyFunction(void);).


//******************************************************************************************************
//**** System header files
//******************************************************************************************************
#include <string.h>
#include <Button.h>
#include <Screen.h>


//******************************************************************************************************
//**** Project header files
//******************************************************************************************************
#include "TextEntryAlert.h"
#include "Colors.h"
#include "NewStrings.h"


//******************************************************************************************************
//**** Constants
//******************************************************************************************************
const float c_text_border_width = 2;
const float c_text_margin = 1;
const float c_item_spacing = 9;
const float c_non_inline_label_spacing = 5;
const float c_inline_label_spacing = 7;
const float c_usual_button_width = 74;


//******************************************************************************************************
//**** BMessage what constants for internal use
//******************************************************************************************************
const uint32 c_button_pressed = 0;


//******************************************************************************************************
//**** TextEntryAlert
//******************************************************************************************************


TextEntryAlert::TextEntryAlert(const char* title, const char* info_text, const char* initial_entry_text,
	const char* button_0_label, const char* button_1_label, bool inline_label, float min_text_box_width,
	int min_text_box_rows, button_width width_style, bool multi_line, const BRect* frame,
	window_look look, window_feel feel, uint32 flags)
: BWindow((frame?*frame:BRect(50,50,60,60)),title,look,feel,flags)
{
	//Get unadjusted sizes of the two text boxes
	BRect info_text_box;
	BRect entry_text_rect;
	const char* strings[2] = {info_text?info_text:"",initial_entry_text?initial_entry_text:""};
	escapement_delta zero_escapements[2] = {{0,0},{0,0}};
	BRect result_rects[2];
	be_plain_font->GetBoundingBoxesForStrings(strings,2,B_SCREEN_METRIC,zero_escapements,result_rects);
	struct font_height plain_font_height;
	be_plain_font->GetHeight(&plain_font_height);
	info_text_box = result_rects[0];
	info_text_box.bottom = info_text_box.top + ceil(plain_font_height.ascent) +
		ceil(plain_font_height.descent);
	entry_text_rect = result_rects[1];
	entry_text_rect.bottom = entry_text_rect.top +
		(ceil(plain_font_height.ascent)+ceil(plain_font_height.descent))*min_text_box_rows +
		ceil(plain_font_height.leading)*(min_text_box_rows-1);
	entry_text_rect.InsetBy(0-(c_text_margin+c_text_border_width),
		0-(c_text_margin+c_text_border_width));
	if(entry_text_rect.Width() < min_text_box_width)
		entry_text_rect.right = entry_text_rect.left+min_text_box_width;
	entry_text_rect.bottom -= 1;			//To make it look like BTextControl

	//Position and create label
	m_label_view = NULL;
	if(info_text)
	{
		info_text_box.OffsetTo(c_item_spacing,c_item_spacing);
		if(inline_label)
			info_text_box.OffsetBy(0,c_text_border_width+c_text_margin);
		else
			info_text_box.OffsetBy(c_non_inline_label_spacing,-2);
		info_text_box.bottom += 2;		//Compensate for offset used by BTextView
		info_text_box.right += 1;
		m_label_view = new BTextView(info_text_box,NULL,info_text_box.OffsetToCopy(0,0),
			B_FOLLOW_LEFT|B_FOLLOW_TOP,B_WILL_DRAW);
		m_label_view->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
		m_label_view->SetText(info_text);
		m_label_view->MakeEditable(false);
		m_label_view->MakeSelectable(false);
		m_label_view->SetWordWrap(false);
	}
	else
	{
		info_text_box.Set(0,0,0,0);
		inline_label = false;
	}

	//Create buttons
	m_buttons[0] = NULL;
	m_buttons[1] = NULL;
	BMessage* message;
	float button_0_width, button_1_width, buttons_height;
	float max_buttons_width = 0;
	if(button_0_label != NULL)
	{
		message = new BMessage(c_button_pressed);
		message->AddInt32("which",0);
		m_buttons[0] = new BButton(BRect(0,0,0,0),button_0_label,button_0_label,message,B_FOLLOW_LEFT|
			B_FOLLOW_BOTTOM);
		m_buttons[0]->GetPreferredSize(&button_0_width,&buttons_height);
		max_buttons_width = button_0_width;
	}
	if(button_1_label != NULL)
	{
		message = new BMessage(c_button_pressed);
		message->AddInt32("which",1);
		m_buttons[1] = new BButton(BRect(0,0,0,0),button_1_label,button_1_label,message,B_FOLLOW_RIGHT|
			B_FOLLOW_BOTTOM);
		m_buttons[1]->GetPreferredSize(&button_1_width,&buttons_height);
		if(max_buttons_width < button_1_width)
			max_buttons_width = button_1_width;
	}
	
	//Position and create text entry box
	if(inline_label)
		entry_text_rect.OffsetTo(info_text_box.right+c_inline_label_spacing,c_item_spacing);
	else
		entry_text_rect.OffsetTo(c_item_spacing,info_text_box.bottom+c_non_inline_label_spacing-4);
		//-5 is to compensate for extra pixels that BeOS adds to the font height
	if(frame != NULL)
	{
		if(entry_text_rect.left + min_text_box_width < frame->Width()-c_item_spacing)
			entry_text_rect.right = frame->Width()+c_item_spacing;
		else
			entry_text_rect.right = entry_text_rect.left + min_text_box_width;
		if(min_text_box_rows > 1)
		{
			float bottom = frame->Height()-c_item_spacing;
			if(button_0_label != NULL || button_1_label != NULL)
				bottom -= (buttons_height+c_item_spacing);
			if(bottom > entry_text_rect.bottom)
				entry_text_rect.bottom = bottom;
		}
	}

	m_text_entry_view = new TextEntryAlertTextEntryView(entry_text_rect.InsetByCopy(c_text_border_width,
		c_text_border_width),multi_line);
	if(initial_entry_text)
		m_text_entry_view->SetText(initial_entry_text);

	//Position the buttons
	if(m_buttons[0] != NULL && m_buttons[1] != NULL)
	{
		float button_left;
		if(inline_label)
			button_left = info_text_box.left;
		else
			button_left = entry_text_rect.left;
		m_buttons[0]->MoveTo(button_left,entry_text_rect.bottom+c_item_spacing);
		if(width_style == B_WIDTH_AS_USUAL)
			m_buttons[0]->ResizeTo(c_usual_button_width,buttons_height);
		else if(width_style == B_WIDTH_FROM_LABEL)
			m_buttons[0]->ResizeTo(button_0_width,buttons_height);
		else //if(width_style == B_WIDTH_FROM_WIDEST)
			m_buttons[0]->ResizeTo(max_buttons_width,buttons_height);
	}
	BButton* right_button = NULL;
	if(m_buttons[1] != NULL)
		right_button = m_buttons[1];
	else if(m_buttons[0] != NULL)
		right_button = m_buttons[0];
	if(right_button != NULL)
	{
		if(width_style == B_WIDTH_AS_USUAL)
			right_button->ResizeTo(c_usual_button_width,buttons_height);
		else if(width_style == B_WIDTH_FROM_LABEL)
			right_button->ResizeTo(right_button==m_buttons[1]?button_1_width:button_0_width,
				buttons_height);
		else //(width_style == B_WIDTH_FROM_WIDEST)
			right_button->ResizeTo(max_buttons_width,buttons_height);
		right_button->MoveTo(entry_text_rect.right-right_button->Frame().Width()+1,
			entry_text_rect.bottom+c_item_spacing);
		if(!multi_line)
			right_button->MakeDefault(true);
	}

	//Resize the window
	float height;
	if(m_buttons[0])
		height = m_buttons[0]->Frame().bottom+c_item_spacing-1;
	else if(m_buttons[1])
		height = m_buttons[1]->Frame().bottom+c_item_spacing-1;
	else
		height = entry_text_rect.bottom+c_item_spacing-1;
	ResizeTo(entry_text_rect.right+c_item_spacing,height);

	BRect bounds = Bounds();
	if(frame == NULL)
	{
		BRect screen_frame = BScreen().Frame();
		MoveTo(ceil((screen_frame.Width()-bounds.Width())/2),
			ceil((screen_frame.Height()-bounds.Height()-19)/2));
	}

	//Create the background view and add the children
	BView* filler = new TextEntryAlertBackgroundView(bounds,entry_text_rect);
	if(m_label_view)
		filler->AddChild(m_label_view);
	filler->AddChild(m_text_entry_view);
	if(m_buttons[0])
		filler->AddChild(m_buttons[0]);
	if(m_buttons[1])
		filler->AddChild(m_buttons[1]);
	AddChild(filler);

	//Complete the setup
	m_invoker = NULL;
	m_done_mutex = B_ERROR;
	m_text_entry_buffer = NULL;
	m_button_pressed = NULL;
	float min_width = c_item_spacing;
	if(m_buttons[0])
		min_width += (m_buttons[0]->Frame().Width() + c_item_spacing);
	if(m_buttons[1])
		min_width += (m_buttons[0]->Frame().Width() + c_item_spacing);
	if(min_width < 120)
		min_width = 120;
	float min_height = entry_text_rect.top;
	min_height += (ceil(plain_font_height.ascent)+ceil(plain_font_height.descent));
	min_height += ((c_text_margin+c_text_border_width)*2 + c_item_spacing);
	if(m_buttons[0] || m_buttons[1])
		min_height += (buttons_height + c_item_spacing);
	min_width -= 2;		//Need this for some reason
	min_height -= 2;
	SetSizeLimits(min_width,100000,min_height,100000);
	AddCommonFilter(new BMessageFilter(B_KEY_DOWN,KeyDownFilterStatic));
	m_shortcut[0] = 0;
	m_shortcut[1] = 0;
}


TextEntryAlert::~TextEntryAlert()
{
	if(m_invoker)
		delete m_invoker;
}


int32 TextEntryAlert::Go(char* text_entry_buffer, int32 buffer_size)
{
	int32 button_pressed = -1;
	m_text_entry_buffer = text_entry_buffer;
	m_button_pressed = &button_pressed;
	m_buffer_size = buffer_size;
	sem_id done_mutex = create_sem(0,"text_entry_alert_done");
	m_done_mutex = done_mutex;
	if(done_mutex < B_NO_ERROR)
	{
		Quit();
		return -1;
	}
	m_text_entry_view->MakeFocus(true);
	m_text_entry_view->SetMaxBytes(buffer_size);
	Show();
	acquire_sem(done_mutex);
	delete_sem(done_mutex);
	return button_pressed;
}


status_t TextEntryAlert::Go(BInvoker *invoker)
{
	m_invoker = invoker;
	m_text_entry_view->MakeFocus(true);
	Show();
	return B_NO_ERROR;
}


void TextEntryAlert::SetShortcut(int32 index, char shortcut)
{
	m_shortcut[index] = shortcut;
}


char TextEntryAlert::Shortcut(int32 index) const
{
	return m_shortcut[index];
}


filter_result TextEntryAlert::KeyDownFilterStatic(BMessage* message, BHandler** target,
	BMessageFilter* filter)
{
	return ((TextEntryAlert*)filter->Looper())->KeyDownFilter(message);
}


filter_result TextEntryAlert::KeyDownFilter(BMessage *message)
{
	if(message->what == B_KEY_DOWN)
	{
		char byte;
		if(message->FindInt8("byte",(int8*)&byte) == B_NO_ERROR && !message->HasInt8("byte",1))
		{
			char space = B_SPACE;
			if(m_shortcut[0] && byte == m_shortcut[0])
			{
				m_buttons[0]->KeyDown(&space,1);
				return B_SKIP_MESSAGE;
			}
			else if(m_shortcut[1] && byte == m_shortcut[1])
			{
				m_buttons[1]->KeyDown(&space,1);
				return B_SKIP_MESSAGE;
			}
		}
	}
	return B_DISPATCH_MESSAGE;
}


BTextView* TextEntryAlert::LabelView(void) const
{
	return m_label_view;
}


BTextView* TextEntryAlert::TextEntryView(void) const
{
	return m_text_entry_view;
}


void TextEntryAlert::SetTabAllowed(bool allow)
{
	m_text_entry_view->SetTabAllowed(allow);
}


bool TextEntryAlert::TabAllowed()
{
	return m_text_entry_view->TabAllowed();
}


void TextEntryAlert::MessageReceived(BMessage* message)
{
	if(message->what == c_button_pressed)
	{
		int32 which;
		if(message->FindInt32("which",&which) == B_NO_ERROR)
		{
			const char* text = m_text_entry_view->Text();
			if(text == NULL)
				text = "";
			if(m_done_mutex < B_NO_ERROR)
			{
				//Asynchronous version: add the necessary fields
				BMessage* message = m_invoker->Message();
				if(message && (message->AddInt32("which",which) == B_NO_ERROR ||
					message->ReplaceInt32("which",which) == B_NO_ERROR) &&
					(message->AddString("entry_text",text) == B_NO_ERROR ||
					message->ReplaceString("entry_text",text) == B_NO_ERROR))
					m_invoker->Invoke();
			}
			else
			{
				//Synchronous version: set the result button and text buffer, then release the thread
				//that created me
				*m_button_pressed = which;
				if(m_text_entry_buffer)
					Strtcpy(m_text_entry_buffer,text,m_buffer_size);
				release_sem(m_done_mutex);
				m_done_mutex = B_ERROR;
			}
			PostMessage(B_QUIT_REQUESTED);
		}
	}
}

void TextEntryAlert::Quit()
{
	//Release the mutex if I'm synchronous and I haven't released it yet
	if(m_done_mutex >= B_NO_ERROR)
		release_sem(m_done_mutex);
	BWindow::Quit();
}


//******************************************************************************************************
//**** TextEntryAlertBackgroundView
//******************************************************************************************************
TextEntryAlertBackgroundView::TextEntryAlertBackgroundView(BRect frame, BRect entry_text_rect)
: BView(frame,NULL,B_FOLLOW_ALL_SIDES,B_WILL_DRAW|B_FRAME_EVENTS)
{
	m_entry_text_rect = entry_text_rect;
	m_cached_bounds = Bounds();
	rgb_color background_color = ui_color(B_PANEL_BACKGROUND_COLOR);
	m_dark_1_color = tint_color(background_color,B_DARKEN_1_TINT);
	m_dark_2_color = tint_color(background_color,B_DARKEN_4_TINT);
	SetViewColor(background_color);
	SetDrawingMode(B_OP_COPY);
}


void TextEntryAlertBackgroundView::Draw(BRect update_rect)
{
	if(update_rect.Intersects(m_entry_text_rect))
	{
		SetHighColor(m_dark_1_color);
		StrokeLine(BPoint(m_entry_text_rect.left,m_entry_text_rect.top),
			BPoint(m_entry_text_rect.right,m_entry_text_rect.top));
		StrokeLine(BPoint(m_entry_text_rect.left,m_entry_text_rect.top+1),
			BPoint(m_entry_text_rect.left,m_entry_text_rect.bottom));
		SetHighColor(White);
		StrokeLine(BPoint(m_entry_text_rect.right,m_entry_text_rect.top+1),
			BPoint(m_entry_text_rect.right,m_entry_text_rect.bottom-1));
		StrokeLine(BPoint(m_entry_text_rect.left+1,m_entry_text_rect.bottom),
			BPoint(m_entry_text_rect.right,m_entry_text_rect.bottom));
		SetHighColor(m_dark_2_color);
		StrokeLine(BPoint(m_entry_text_rect.left+1,m_entry_text_rect.top+1),
			BPoint(m_entry_text_rect.right-1,m_entry_text_rect.top+1));
		StrokeLine(BPoint(m_entry_text_rect.left+1,m_entry_text_rect.top+2),
			BPoint(m_entry_text_rect.left+1,m_entry_text_rect.bottom-1));
	}
}


void TextEntryAlertBackgroundView::FrameResized(float width, float heigh)
{
	BRect new_bounds = Bounds();
	float width_delta = new_bounds.right - m_cached_bounds.right;
	float height_delta = new_bounds.bottom - m_cached_bounds.bottom;
	BRect new_entry_text_rect = m_entry_text_rect;
	new_entry_text_rect.right += width_delta;
	new_entry_text_rect.bottom += height_delta;

	float right_min = m_entry_text_rect.right;
	if(right_min > new_entry_text_rect.right)
		right_min = new_entry_text_rect.right;
	float right_max = m_entry_text_rect.right;
	if(right_max < new_entry_text_rect.right)
		right_max = new_entry_text_rect.right;
	float bottom_min = m_entry_text_rect.bottom;
	if(bottom_min > new_entry_text_rect.bottom)
		bottom_min = new_entry_text_rect.bottom;
	float bottom_max = m_entry_text_rect.bottom;
	if(bottom_max < new_entry_text_rect.bottom)
		bottom_max = new_entry_text_rect.bottom;

	if(new_entry_text_rect.right != m_entry_text_rect.right)
		Invalidate(BRect(right_min-1,new_entry_text_rect.top,right_max,bottom_max));
	if(new_entry_text_rect.bottom != m_entry_text_rect.bottom)
		Invalidate(BRect(new_entry_text_rect.left,bottom_min-1,right_max,bottom_max));

	m_entry_text_rect = new_entry_text_rect;
	m_cached_bounds = new_bounds;
}


//******************************************************************************************************
//**** TextEntryAlertTextEntryView
//******************************************************************************************************
TextEntryAlertTextEntryView::TextEntryAlertTextEntryView(BRect frame, bool multi_line)
: BTextView(frame,NULL,frame.OffsetToCopy(0,0).InsetByCopy(c_text_margin,c_text_margin),
	multi_line?B_FOLLOW_ALL_SIDES:B_FOLLOW_LEFT_RIGHT,B_WILL_DRAW|B_NAVIGABLE)
{
	m_tab_allowed = false;
	m_multi_line = multi_line;
	SetViewColor(White);
	SetWordWrap(true);
}


void TextEntryAlertTextEntryView::SetTabAllowed(bool allow)
{
	m_tab_allowed = allow;
}


void TextEntryAlertTextEntryView::KeyDown(const char* bytes, int32 num_bytes)
{
	if(num_bytes == 1 && (bytes[0] == B_TAB && (!m_tab_allowed)) || (bytes[0] == B_ENTER && (!m_multi_line)))
	{
		BView::KeyDown(bytes,num_bytes);
		return;
	}
	BTextView::KeyDown(bytes,num_bytes);
}


void TextEntryAlertTextEntryView::AttachedToWindow()
{
	BTextView::AttachedToWindow();
	const char* text = Text();
	if(text)
	{
		int32 length = strlen(text);
		Select(length,length);
	}
}


