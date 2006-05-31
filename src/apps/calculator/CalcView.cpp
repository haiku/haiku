/*
 * Copyright (c) 2006, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Peter Wagner <pwagner@stanfordalumni.org>
 */
#include <iostream.h>
#include <ctype.h>
#include <string.h>

#include <Alert.h>
#include <Button.h>
#include <StringView.h>
#include <Beep.h>
#include <Dragger.h>
#include <TextView.h>
#include <View.h>
#include <Clipboard.h>

#include "CalcView.h"
#include "FrameView.h"


const int border = 8;
const int key_border = 4;
const int key_height = 23;
const int key_width = 32;


void CalcView::MouseDown(BPoint where)
{
	BView::MouseDown(where);
	MakeFocus();
	ChangeHighlight();
}


void CalcView::ChangeHighlight()
{
	int state = state_unknown;

	BWindow *wind = Window();
	if (wind != NULL)
		state = (wind->IsActive() && IsFocus()) ? state_active : state_inactive;

	if (state != current_highlight) {
		rgb_color on = {204,204,255};
		rgb_color off = {204*9/10, 204*9/10, 255*9/10};

		if (state == state_inactive) {
			lcd_frame->SetViewColor(off);
			lcd->SetLowColor(off);
			lcd->SetViewColor(off);
	//		lcd->SetHighColor(102,102,102);
		}
		else {
			lcd_frame->SetViewColor(on);
			lcd->SetLowColor(on);
			lcd->SetViewColor(on);
	//		lcd->SetHighColor(51,51,51);
		}
		lcd_frame->Invalidate();
		lcd->Invalidate();

		current_highlight = state;
	}
}


void CalcView::WindowActivated(bool state)
{
	BView::WindowActivated(state);

	ChangeHighlight();
}


void CalcView::MakeFocus(bool focusState)
{
	BView::MakeFocus(focusState);
	
	ChangeHighlight();
}


CalcView::CalcView(BRect rect)
 : BView(rect, "Calculator", B_FOLLOW_NONE, B_WILL_DRAW),
   calc(15)
{
	Init();

	BDragger *dragger;
	dragger = new BDragger(BRect(0,0,7,7), this, 0);
	AddChild(dragger);

	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	BRect r;
	r.left = border;
	r.right = r.left + key_border*8 + key_width*7;
	r.top = border-1;
	r.bottom = r.top + 40;
	r.InsetBy(-1,0);
	lcd_frame = new FrameView(r, "lcd frame", B_FOLLOW_LEFT_RIGHT|B_FOLLOW_TOP, 1);
	AddChild(lcd_frame);

	BRect r2;
	r2 = lcd_frame->Bounds();
	r2.InsetBy(5,2);
	r2.OffsetBy(3,1);
	lcd = new BStringView(r2, "lcd", "0", B_FOLLOW_ALL_SIDES);
	lcd->SetFont(be_plain_font/*"Arial MT"*/);
	lcd->SetFontSize(30);
	lcd->SetAlignment(B_ALIGN_RIGHT);
	lcd_frame->AddChild(lcd);

	static FrameView::ClusterInfo trig_button_info[] =
	{ {"sin", 'sin'}, {"cos", 'cos'},
	  {"tan", 'tan'}, {0}, {0},
	  {"inv", 'trgA'}, {"deg", 'trgM'}
	};
	FrameView *trig_frame
		= new FrameView(BPoint(8,57), "trig area", B_FOLLOW_NONE,
					7,1, 1, key_border, key_height, key_width,
					trig_button_info);
	AddChild(trig_frame);

	static FrameView::ClusterInfo const_button_info[] =
	{ {"-/+", '-/+'}, {"exp", 'exp'},
	  {"e", 'e'}, {"pi", 'pi'}, {0},
	  {"(", '('}, {")", ')'}
	};
	FrameView *const_frame
		= new FrameView(BPoint(8,97), "const area", B_FOLLOW_NONE,
					7,1, 1, key_border, key_height, key_width,
					const_button_info);
	AddChild(const_frame);



	static FrameView::ClusterInfo keypad_button_info[] =
	{ {"C",'clr'}, {"=",'='}, {"/",'/'}, {"*",'*'},
	  {"7",'7'}, {"8",'8'}, {"9",'9'}, {"-",'-'},
	  {"4",'4'}, {"5",'5'}, {"6",'6'}, {"+",'+'},
	  {"1",'1'}, {"2",'2'}, {"3",'3'}, {"=",'entr'},
	  {"0",'0'}, {0}, {".",'.'}, {0}
	};
	FrameView *keypad_frame
		= new FrameView(BPoint(8,137), "keypad area", B_FOLLOW_NONE,
					4,5, 1, key_border, key_height, key_width,
					keypad_button_info);
	AddChild(keypad_frame);
	keypad_frame->FindView("entr")->ResizeBy(0, key_height+key_border);
	keypad_frame->FindView("0")->ResizeBy(key_width+key_border, 0);


	static FrameView::ClusterInfo func_button_info[] =
	{ {"x^y", '^'}, {"x^1/y", '^1/y'},
	  {"x^2", 'x^2'}, {"sqrt", 'sqrt'},
	  {"10^x", '10^x'}, {"log", 'log'},
	  {"e^x", 'e^x'}, {"ln x", 'ln x'},
	  {"x!", '!'}, {"1/x", '1/x'}
	};
	FrameView *func_frame
		= new FrameView(BPoint(188,137), "func area", B_FOLLOW_NONE,
					2,5, 1, key_border, key_height, key_width,
					func_button_info);
	AddChild(func_frame);
}


CalcView::CalcView(BMessage *msg)
 : BView(msg),
   calc(15)
{
	Init();

	is_replicant = true;

	lcd_frame = (FrameView *)FindView("lcd frame");
	lcd = (BStringView *)FindView("lcd");

	lcd->SetText("0");
}


void CalcView::Init()
{
	is_replicant = false;
	current_highlight = state_unknown;

	binary_sign_view = NULL;
	binary_exponent_view = NULL;
	binary_number_view = NULL;
}


void CalcView::Draw(BRect updateRect)
{
	BView::Draw(updateRect);

	if (is_replicant)
	{
		const float bevel = 2;
		BRect r = Bounds();
		r.right -= (bevel - 1);
		r.bottom -= (bevel - 1);
		SetPenSize(bevel);
		StrokeRect(r);
		SetPenSize(1);
	}
}


BView * CalcView::AddExtra()
{
	BRect r2;

	BRect r = Frame();
	r.top = r.bottom;
	r.bottom = r.top + ExtraHeight();
	BView *view = new BView(r, "Calculator Extra", 0, B_WILL_DRAW);
	view->SetViewColor(153,153,153);

	r = view->Bounds();
	r.InsetBy(border, 1);
	r.left -= 1;
	r.bottom = r.top + 16;
	r.right = r.left + 8+2;
	FrameView *binary_frame = new FrameView(r, NULL, 0, 1);
	view->AddChild(binary_frame);
	r2 = binary_frame->Bounds();
	r2.InsetBy(2,2);
	r2.OffsetBy(1,1);
	binary_sign_view = new BStringView(r2, "binary sign view", "", 0);
	binary_sign_view->SetAlignment(B_ALIGN_CENTER);
	binary_sign_view->SetViewColor(binary_frame->ViewColor());
	binary_frame->AddChild(binary_sign_view);

	r.OffsetBy(r.Width()+3, 0);
	r.right = r.left + 72;
	binary_frame = new FrameView(r, NULL, 0, 1);
	view->AddChild(binary_frame);
	r2 = binary_frame->Bounds();
	r2.InsetBy(2,2);
	r2.OffsetBy(1,1);
	binary_exponent_view = new BStringView(r2, "binary exponent view", "", 0);
	binary_exponent_view->SetAlignment(B_ALIGN_CENTER);
	binary_exponent_view->SetViewColor(binary_frame->ViewColor());
	binary_frame->AddChild(binary_exponent_view);

	r.OffsetBy(r.Width()+3, 0);
	r.right = Bounds().Width() - border + 1;
	binary_frame = new FrameView(r, NULL, 0, 1);
	view->AddChild(binary_frame);
	r2 = binary_frame->Bounds();
	r2.InsetBy(2,2);
	r2.OffsetBy(1,1);
	binary_number_view = new BStringView(r2, "binary number view", "", 0);
	binary_number_view->SetAlignment(B_ALIGN_CENTER);
	binary_number_view->SetViewColor(binary_frame->ViewColor());
	binary_frame->AddChild(binary_number_view);


	r = view->Bounds();
	r.InsetBy(border, 0);
	r.top = binary_frame->Frame().bottom + 3;
	r.bottom = r.top + 16;
	r.InsetBy(-1,0);
	FrameView *about_frame = new FrameView(r, NULL, 0, 1);
	view->AddChild(about_frame);
	r2 = about_frame->Bounds();
	r2.InsetBy(2,2);
	r2.OffsetBy(0,1);
	BStringView *peter = new BStringView(r2, NULL,
							"Brought to you by Peter Wagner", 0);
	peter->SetAlignment(B_ALIGN_CENTER);
	peter->SetViewColor(about_frame->ViewColor());
	about_frame->AddChild(peter);

	return view;
}


status_t CalcView::Archive(BMessage *msg, bool deep) const
{
	BView::Archive(msg, deep);
//	msg->AddString("class", "CalcView");
	msg->AddString("add_on", "application/x-vnd.Be-calculator");
	return B_OK;
}


BArchivable *CalcView::Instantiate(BMessage *msg)
{
	if (validate_instantiation(msg, "CalcView"))
		return new CalcView(msg);
	else
		return NULL;
}


CalcView::~CalcView()
{
}


static void ConnectButtonMessages(BView *target, const BView *which)
{
	BView *child = which->ChildAt(0);
	while (child != NULL)
	{
		BControl * button = (BControl *)child;
		button->SetTarget(target);
		child = child->NextSibling();
	}
}


void CalcView::AllAttached()
{
	BView::AllAttached();

	lcd->SetHighColor(51,51,51);
	MakeFocus();
	ChangeHighlight();
}


void CalcView::AttachedToWindow()
{
	BView::AttachedToWindow();

	// Make buttons send their messages to this view instead of the 
	// [default] enclosing window.
	ConnectButtonMessages(this, FindView("trig area"));
	ConnectButtonMessages(this, FindView("const area"));
	ConnectButtonMessages(this, FindView("keypad area"));
	ConnectButtonMessages(this, FindView("func area"));
}


void CalcView::KeyDown(const char *bytes, int32 numBytes)
{
	for (int i=0; i<numBytes; i++) 	{
		char key = bytes[i];
		BButton *button = NULL;

		switch(key) {
			case '¸':
			case '¹':
			case 'p':
			case 'P':
				button = (BButton *)FindView("pi");
				break;
			case 'C':
			case 'c':
				button = (BButton *)FindView("clr");
				break;
			case 'e':
			case 'E':
				button = (BButton *)FindView("exp");
				break;
			case '`':
				button = (BButton *)FindView("-/+");
				break;
			case '(':
			case ')':
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
			case '=':
			case '/':
			case '*':
			case '-':
			case '+':
			case '.':
			case '^':
			case '!':
				char name[2];
				if (isalpha(key)) key = tolower(key);
				name[0] = key;
				name[1] = '\0';
				button = (BButton *)FindView(name);
				break;
			case 10:
				button = (BButton *)FindView("entr");
				break;
			case 127:
				button = (BButton *)FindView(".");
				break;
			default:
				button = button;
				break;
		}

		if (button != NULL) {
			button->SetValue(1);
			snooze(100000);
			button->SetValue(0);
			snooze(20000);
			BMessage msg(*button->Message());
			SendKeystroke(msg.what);
		}
	}

}


static void DoubleToBinary(double *dPtr, char *str)
{
	unsigned char *p = (unsigned char *)dPtr;
	str[sizeof(double)*8] = '\0';
	const int bits = sizeof(double) * 8;

	unsigned short endian_test = 1;
	
	if(((unsigned char *)&endian_test)[0] != 0) {
		// little-endian
		for (int i=0; i<bits; i++)
			str[i] = (p[(sizeof(double) - i/8) - 1] & (128 >> (i&7))) ? '1' : '0';
	}
	else {
		// Big endian
		for (int i=0; i<bits; i++)
			str[i] = (p[i/8] & (128 >> (i&7))) ? '1' : '0';
	}
}


static char * Mid(const char *str, int start, int stop, char *str_out)
{
	char *s = str_out;

	for (int i=start; i<stop; i++)
		*s++ = str[i];
	
	*s = '\0';

	return str_out;
}


void CalcView::SendKeystroke(ulong key)
{
	calc.DoCommand(key);

	BButton *trig_inverse = (BButton *)FindView("trgA");
	trig_inverse->SetValue(calc.TrigInverseActive()?1:0);

	BButton *trig_mode = (BButton *)FindView("trgM");
	trig_mode->SetLabel(calc.TrigIsRadians()?"rad":"deg");


	// Update LCD display.
	char engine_str[64];
	calc.GetValue(engine_str);
	lcd->SetText(engine_str);


	if (binary_sign_view && binary_exponent_view && binary_number_view) {
		// Update binary representation of current display.
		double d = calc.GetValue();
		char str[300], str2[300];
		DoubleToBinary(&d, str);
		binary_sign_view->SetText(Mid(str, 0,1, str2));
		binary_exponent_view->SetText(Mid(str, 1,12, str2));
	
		Mid(str, 13,sizeof(double)*8, str2);
		while(binary_number_view->StringWidth(str2) >= binary_number_view->Bounds().Width()
			  && strlen(str2) > 24) {
			str2[20] = str2[21] = str2[22] = '.';
			strcpy(str2+23, str2+24);
		}
		binary_number_view->SetText(str2);
	}
}


static bool IsSystemCommand(unsigned long what)
{
	for (int zz=0; zz<4; zz++) {
		char c = (char)(what >> (zz*8));
		if (!isupper(c) && c!='_') return false;
	}
	
	return true;
}


void CalcView::About()
{
	(new BAlert("About Calculator", "Calculator (the Replicant version)\n\n  Brought to you by Peter Wagner.\n  Copyright 1998.","OK"))->Go();
}


void CalcView::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case B_ABOUT_REQUESTED:
			CalcView::About();
			break;
		case B_COPY: {
				double d;
				char str[64];
				d = calc.GetValue(str);
				if (be_clipboard->Lock())
				{
					be_clipboard->Clear();
					BMessage *clipper = be_clipboard->Data();
					clipper->AddDouble("value", d);
					clipper->AddData("text/plain", 'MIME', str, strlen(str));
					be_clipboard->Commit();
					be_clipboard->Unlock();
				}
			}
			break;
		case B_PASTE: {
				const void *data = NULL;
				ssize_t sz;
				if (be_clipboard->Lock()) {
					BMessage *clipper = be_clipboard->Data();
					status_t status = clipper->FindData("text/plain", 'MIME', &data, &sz);
					if (status == B_OK && data!=NULL)
						KeyDown((const char *)data, (int32)sz);
/*
					// What the heck was on the clipboard !??!
					{
						char  *name;
						uint32  type;
						int32   count;
						char typeS[5];
						for ( int32 i = 0;
							  clipper->GetInfo(B_ANY_TYPE, i, &name, &type, &count) == B_OK;
							  i++ )
						{
							for (int z=0; z<4; z++)
							{
								typeS[z] = (char)(type >> 24);
								type = type << 8;
							}
							cout << i << ": " << name << " " << typeS << "\n";
						}
					}
*/
					be_clipboard->Unlock();
				}
			}
			break;
		default:
			if (IsSystemCommand(message->what))
				BView::MessageReceived(message);
			else {
				SendKeystroke(message->what);
				MakeFocus();
				ChangeHighlight();
			}
			break;
	}

/*
	// What the heck was that message?!!	
	char str[5];
	str[4] = '\0';
	ulong msg = message->what;
	for (int i=0; i<4; i++)
	{
		str[i] = (char)(msg>>24);
		msg = msg << 8;
		if (str[i] == '\0') str[i] = '-';
	}
	lcd->SetText(str);
*/

}


bool CalcView::TextWidthOK(const char *str)
{
	return (lcd->StringWidth(str) < lcd->Bounds().Width()) ? true : false;
}
