/*! \file FontSelectionView.h
    \brief Header for the FontSelectionView class.
    
*/

#ifndef FONT_SELECTION_VIEW_H

	#define FONT_SELECTION_VIEW_H
	
	/*!
	 *  Constant to define that the view is a plain view.
	 */
	#define PLAIN_FONT_SELECTION_VIEW 1
	/*!
	 *  Constant to define that the view is a bold view.
	 */
	#define BOLD_FONT_SELECTION_VIEW 2
	/*!
	 *  Constant to define that the view is a fixed view.
	 */
	#define FIXED_FONT_SELECTION_VIEW 3
	
	/*!
	 *  The plain size menu was changed.
	 */
	#define PLAIN_SIZE_CHANGED_MSG 'plsz'
	/*!
	 *  The bold size menu was changed.
	 */
	#define BOLD_SIZE_CHANGED_MSG 'blsz'
	/*!
	 *  The fixed size menu was changed.
	 */
	#define FIXED_SIZE_CHANGED_MSG 'fxsz'
	
	/*!
	 *  The plain font menu was changed.
	 */
	#define PLAIN_FONT_CHANGED_MSG 'plfn'
	/*!
	 *  The bold font menu was changed.
	 */
	#define BOLD_FONT_CHANGED_MSG 'blfn'
	/*!
	 *  The fixed font menu was changed.
	 */
	#define FIXED_FONT_CHANGED_MSG 'fxfn'
	
	/*!
	 *  The plain style menu was changed.
	 */
	#define PLAIN_STYLE_CHANGED_MSG 'plst'
	/*!
	 *  The bold style menu was changed.
	 */
	#define BOLD_STYLE_CHANGED_MSG 'blst'
	/*!
	 *  The fixed style menu was changed.
	 */
	#define FIXED_STYLE_CHANGED_MSG 'fxst'
	
	#ifndef _VIEW_H
		
		#include <View.h>
	
	#endif
	
	#ifndef _BOX_H
	
		#include <Box.h>
		
	#endif
	
	#ifndef _STRING_VIEW_H
	
		#include <StringView.h>
		
	#endif
	
	#ifndef _POP_UP_MENU_H
	
		#include <PopUpMenu.h>
		
	#endif
	
	#ifndef _MENU_FIELD_H
	
		#include <MenuField.h>
		
	#endif
	
	#ifndef _MENU_ITEM_H
	
		#include <MenuItem.h>
		
	#endif
	#ifndef _STDIO_H
	
		#include <stdio.h>
		
	#endif
	
	class FontSelectionView : public BView{
	
		public:
			
			FontSelectionView(BRect rect, const char *name, int type);
			void SetTestTextFont(BFont *fnt);
			BFont GetTestTextFont();
			float GetSelectedSize();
			void GetSelectedFont(font_family *family);
			void GetSelectedStyle(font_style *style);
			void UpdateFontSelectionFromStyle();
			void UpdateFontSelection();
			void buildMenus();
			void emptyMenus();
			void resetToDefaults();
			void revertToOriginal();
			
		private:
		
			/**
			 * The test text display view.
			 */
			BStringView *testText;
			
			/**
			 * The font menu.
			 */
			BPopUpMenu *fontList;
			
			/**
			 * The size menu.
			 */
			BPopUpMenu *sizeList;
			
			/**
			 * The minimum font size allowed.
			 */
			int minSizeIndex;
			
			/**
			 * The maximum font size allowed.
			 */
			int maxSizeIndex;
			
			/**
			 * The message sent when the size menu is changed.
			 */
			uint32 setSizeChangedMessage;
			
			/**
			 * The message sent when the font menu is changed.
			 */
			uint32 setFontChangedMessage;
			
			/**
			 * The message sent when the style menu is changed.
			 */
			uint32 setStyleChangedMessage;
			
			/**
			 * The label that shows what type the view is.
			 */
			char typeLabel[30];
			
			/**
			 * The original font, set when the view is instantiated.
			 */
			BFont origFont;
			
			/**
			 * The current font.
			 */
			BFont workingFont;
			
			/**
			 * The default font.
			 */
			BFont *defaultFont;
			
			void emptyMenu(BPopUpMenu *m);
			void UpdateFontSelection(BFont *fnt);
			
	
	};
	
#endif
