#ifndef ZOIDBERG_MAIL_ERRORLOGWINDOW_H
#define ZOIDBERG_MAIL_ERRORLOGWINDOW_H

#include <Window.h>
#include <Alert.h>

class ErrorLogWindow : public BWindow {
public:
		ErrorLogWindow(BRect rect, const char *name, window_type type);
		
		void AddError(alert_type type,const char *message,const char *tag = NULL,bool timestamp = true);
		
		bool QuitRequested();
		void FrameResized(float new_width, float new_height);
	
private:
		BView *view;
		bool	fIsRunning;
};

#endif // ZOIDBERG_MAIL_ERRORLOGWINDOW_H
