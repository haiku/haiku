/* PoorManLoggingView.h
 *
 *	Philip Harrison
 *	Started: 5/12/2004
 *	Version: 0.1
 */
 
#ifndef POOR_MAN_LOGGING_VIEW_H
#define POOR_MAN_LOGGING_VIEW_H

#include <View.h>
#include <TextControl.h>
#include <Button.h>
#include <CheckBox.h>


class PoorManLoggingView: public BView
{
public:
				PoorManLoggingView(BRect, const char *name);
				
		void	SetLogConsoleValue(bool state) {if (state) logConsole->SetValue(B_CONTROL_ON); 
												else logConsole->SetValue(B_CONTROL_OFF); }
		bool	LogConsoleValue() { return (logConsole->Value() == B_CONTROL_ON) ? true : false; }
		void	SetLogFileValue(bool state) {if (state) logFile->SetValue(B_CONTROL_ON); 
												else logFile->SetValue(B_CONTROL_OFF); }
		bool	LogFileValue() { return (logFile->Value() == B_CONTROL_ON) ? true : false; }
const	char *	LogFileName() { return logFileName->Text(); }
		void	SetLogFileName(const char * log) { logFileName->SetText(log); }
private:
		// Logging Tab

			// Console Logging
			BCheckBox		*	logConsole;
			// File Logging
			BCheckBox		*	logFile;
			BTextControl	*	logFileName;
			BButton			*	createLogFile;
};

#endif
