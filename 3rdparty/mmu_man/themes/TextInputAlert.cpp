#include "TextInputAlert.h"


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
}


TextInputAlert::~TextInputAlert()
{
}


