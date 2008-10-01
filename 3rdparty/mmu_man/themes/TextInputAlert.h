#ifndef TEXT_INPUT_ALERT_H
#define TEXT_INPUT_ALERT_H

#include <Alert.h>
#include <TextControl.h>

class TextInputAlert : public BAlert {
public:
		TextInputAlert(const char *title, 
						const char *text, 
						const char *initial, /* initial input value */
						const char *button0Label, 
						const char *button1Label = NULL, 
						const char *button2Label = NULL, 
						button_width widthStyle = B_WIDTH_AS_USUAL,
						alert_type type = B_INFO_ALERT);
		virtual ~TextInputAlert();
		virtual void Show();

		const char *Text() const { return fText->Text(); };
		BTextControl *TextControl() const { return fText; };

		
private:
		BTextControl *fText;
};

#endif /* TEXT_INPUT_ALERT_H */

