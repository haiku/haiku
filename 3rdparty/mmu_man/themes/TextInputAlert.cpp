//HACK :P
#define private public
#include <Alert.h>
#undef private

#include "TextInputAlert.h"

#define TEXT_HEIGHT 25


TextInputAlert::TextInputAlert(const char *title, 
						const char *text, 
						const char *initial, /* initial input value */
						const char *button0Label, 
						const char *button1Label, 
						const char *button2Label, 
						button_width widthStyle,
						alert_type type)
	: BAlert(title, text, button0Label, button1Label, button2Label, widthStyle, type)
{
	ResizeBy(0,TEXT_HEIGHT);
	BRect f = Bounds();
	f.InsetBySelf(TEXT_HEIGHT, f.Height()/4 - TEXT_HEIGHT/2);
	f.left *= 3;
	fText = new BTextControl(f, "text", "Name:", initial, NULL);
	fText->SetDivider(f.Width()/3);
	ChildAt(0)->AddChild(fText);
	TextView()->Hide();
	fText->SetViewColor(ChildAt(0)->ViewColor());
	fText->SetLowColor(ChildAt(0)->LowColor());
	fText->TextView()->SelectAll();
}


TextInputAlert::~TextInputAlert()
{
}


void
TextInputAlert::Show()
{
	BAlert::Show();
}


