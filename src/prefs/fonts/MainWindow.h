/*
 * Copyright 2001-2005, Haiku.
 * Distributed under the terms of the MIT License.
 *
 */
#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <Application.h>
#include <Window.h>
#include <TabView.h>
#include <Box.h>

#include "FontView.h"
#include "CacheView.h"
#include "ButtonView.h"
#include "FontsSettings.h"
	
class MainWindow : public BWindow
{
public:
					MainWindow(void); 
	virtual	bool	QuitRequested(void);
	virtual	void	MessageReceived(BMessage *message);
			
private:
		
	void updateSize(FontSelectionView *theView);
	void updateFont(FontSelectionView *theView);
	void updateStyle(FontSelectionView *theView);
	
	FontView	*fSelectorView;
	ButtonView	*fButtonView;
	CacheView	*fCacheView;
	
	FontsSettings fSettings;
};

#endif

