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
class BListView;
class BScrollView;
class LocalDevice;
class DiscoveryAgent;
class DiscoveryListener;

class InquiryPanel : public BWindow 
{
public:
			InquiryPanel(BRect frame, LocalDevice* lDevice = NULL); 
	bool	QuitRequested(void);
	void	MessageReceived(BMessage *message);
	
private:		
	BStatusBar*				fScanProgress;
	BButton*				fAddButton;
	BButton*				fInquiryButton;
	BTextView*				fMessage;
	BListView*				fRemoteList;
	BScrollView*			fScrollView;
	BMessage*				fRetrieveMessage;
	BMessage*				fSecondsMessage;
	BMessenger				fMessenger;
	
	bool					fScanning;
	bool					fRetrieving;
	LocalDevice*			fLocalDevice;
	DiscoveryAgent*			fDiscoveryAgent;
	DiscoveryListener*		fDiscoveryListener;

	void UpdateListStatus(void);

	rgb_color				activeColor;
};

#endif
