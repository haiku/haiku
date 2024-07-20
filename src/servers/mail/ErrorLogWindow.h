#ifndef ZOIDBERG_MAIL_ERRORLOGWINDOW_H
#define ZOIDBERG_MAIL_ERRORLOGWINDOW_H

#include <Window.h>
#include <Alert.h>

class ErrorLogWindow : public BWindow {
public:
		ErrorLogWindow(BRect rect, const char *name, window_type type);

		void AddError(alert_type type, const char *message, const char *tag = NULL, 
			bool timestamp = true);

		bool QuitRequested();
		void FrameResized(float new_width, float new_height);

		void MessageReceived(BMessage* message);

private:
		void SetColumnColors(rgb_color base);

private:
		BView *view;
		bool	fIsRunning;

		rgb_color		fTextColor;
		rgb_color		fColumnColor;
		rgb_color		fColumnAlternateColor;
};

#endif // ZOIDBERG_MAIL_ERRORLOGWINDOW_H
