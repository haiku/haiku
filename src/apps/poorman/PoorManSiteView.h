/* PoorManView.h
 *
 *	Philip Harrison
 *	Started: 5/07/2004
 *	Version: 0.1
 */
 
#ifndef POOR_MAN_SITE_VIEW_H
#define POOR_MAN_SITE_VIEW_H

#include <View.h>
#include <TextControl.h>
#include <Button.h>
#include <CheckBox.h>


class PoorManSiteView: public BView {
public:
							PoorManSiteView(const char *name);

	void SetSendDirValue(bool state)
	{
		if (state)
			fSendDir->SetValue(B_CONTROL_ON);
		else
			fSendDir->SetValue(B_CONTROL_OFF);
	}

	bool SendDirValue()
	{
		return (fSendDir->Value() == B_CONTROL_ON);
	}

	const char* IndexFileName()
	{
		return fIndexFileName->Text();
	}

	void SetIndexFileName(const char* name)
	{
		fIndexFileName->SetText(name);
	}

	const char* WebDir()
	{
		return fWebDir->Text();
	}

	void SetWebDir(const char* dir)
	{
		fWebDir->SetText(dir);
	}

private:
		// Site Tab
			// Web Site Location
			BTextControl*	fWebDir;
			BTextControl*	fIndexFileName;
			BButton*		fSelectWebDir;
			
			// Web Site Options
			BCheckBox*		fSendDir;
			
};

#endif
