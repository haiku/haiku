/*
 * Copyright 2008-09, Oliver Ruiz Dorantes, <oliver.ruiz.dorantes_at_gmail.com>
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef INQUIRY_WINDOW_H
#define INQUIRY_WINDOW_H

#include <Application.h>
#include <Button.h>
#include <Window.h>
#include <Message.h>
#include <TabView.h>

class BStatusBar;
class BButton;
class BTextView;

class InquiryPanel : public BWindow 
{
public:
			InquiryPanel(BRect frame); 
	bool	QuitRequested(void);
	void	MessageReceived(BMessage *message);
	
private:		
		BStatusBar*				fScanProgress;
		BButton*				fAddButton;
		BButton*				fInquiryButton;
		BTextView*				fMessage;

};

#endif
