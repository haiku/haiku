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


class PoorManLoggingView: public BView {
public:
							PoorManLoggingView(const char* name);
				
			void			SetLogConsoleValue(bool state)
								{ if (state)
									fLogConsole->SetValue(B_CONTROL_ON);
									else fLogConsole->SetValue(B_CONTROL_OFF); }
			bool			LogConsoleValue()
								{ return (fLogConsole->Value() == B_CONTROL_ON)
									? true : false; }
			void			SetLogFileValue(bool state)
								{ if (state) fLogFile->SetValue(B_CONTROL_ON); 
									else fLogFile->SetValue(B_CONTROL_OFF); }
			bool			LogFileValue()
								{ return (fLogFile->Value() == B_CONTROL_ON)
									? true : false; }
			const char*		LogFileName()
								{ return fLogFileName->Text(); }
			void			SetLogFileName(const char* log)
								{ fLogFileName->SetText(log); }

private:
			// Logging Tab
			// Console Logging
			BCheckBox*		fLogConsole;
			// File Logging
			BCheckBox*		fLogFile;
			BTextControl*	fLogFileName;
			BButton	*		fCreateLogFile;
};

#endif
