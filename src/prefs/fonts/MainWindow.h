/*! \file MainWindow.h
    \brief Header for the MainWindow class.
    
*/

#ifndef MAIN_WINDOW_H

	#define MAIN_WINDOW_H

	#ifndef _APPLICATION_H
	
		#include <Application.h>
	
	#endif
	#ifndef _WINDOW_H
			
		#include <Window.h>
	
	#endif
	#ifndef _TAB_VIEW_H
		
		#include <TabView.h>
		
	#endif

	#ifndef FONT_VIEW_H
	
		#include "FontView.h"
		
	#endif
	
	#ifndef CACHE_VIEW_H
	
		#include "CacheView.h"
		
	#endif
	
	#ifndef BUTTON_VIEW_H
	
		#include "ButtonView.h"
		
	#endif
		
	#ifndef _BOX_H
	
		#include <Box.h>
		
	#endif
	
	class MainWindow : public BWindow{
	
		public:
			
			MainWindow(BRect frame); 
			virtual	bool QuitRequested();
			virtual void MessageReceived(BMessage *message);
			
		private:
		
			/**
			 * The panel that shows the font selection views.
			 */
			FontView *fontPanel;
			
			/**
			 * The panel that holds the default, revert and rescan buttons.
			 */
			ButtonView *buttonView;
			
			/**
			 * The panel that holds the sliders for adjusting the cache sizes.
			 */
			CacheView *cachePanel;
	
			void updateSize(FontSelectionView *theView);
			void updateFont(FontSelectionView *theView);
			void updateStyle(FontSelectionView *theView);
			
	};

#endif

