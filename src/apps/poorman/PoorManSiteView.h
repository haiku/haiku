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


class PoorManSiteView: public BView
{
public:
				PoorManSiteView(BRect, const char *name);
		void	SetSendDirValue(bool state) {if (state) sendDir->SetValue(B_CONTROL_ON); 
												else sendDir->SetValue(B_CONTROL_OFF); }
		bool	SendDirValue()	{ return (sendDir->Value() == B_CONTROL_ON) ? true : false; }
const	char *	IndexFileName()	{ return indexFileName->Text(); }
		void	SetIndexFileName(const char * name) { indexFileName->SetText(name); }
const	char *	WebDir() 		{ return webDir->Text(); }
		void	SetWebDir(const char * dir) { webDir->SetText(dir); }
private:
		// Site Tab
			// Web Site Location
			BTextControl	*	webDir;
			BTextControl	*	indexFileName;
			BButton			*	selectWebDir;
			
			// Web Site Options
			BCheckBox		*	sendDir;
			
};

#endif
