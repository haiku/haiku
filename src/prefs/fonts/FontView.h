/*! \file FontView.h
    \brief Header for the FontView class.
    
*/

#ifndef FONT_VIEW_H

	#define FONT_VIEW_H
	
	#ifndef _VIEW_H
		
		#include <View.h>
	
	#endif
	
	#ifndef FONT_SELECTION_VIEW_H
	
		#include "FontSelectionView.h"
		
	#endif
		
	#ifndef _BOX_H
	
		#include <Box.h>
		
	#endif
	
	class FontView : public BView{
	
		public:
			
			FontView(BRect frame); 
			FontSelectionView *plainSelectionView;
			FontSelectionView *boldSelectionView;
			FontSelectionView *fixedSelectionView;
			void buildMenus();
			void emptyMenus();
			void resetToDefaults();
			void revertToOriginal();
			
	};
	
#endif
