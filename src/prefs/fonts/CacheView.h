/*! \file CacheView.h
 *  \brief Header file for the CacheView class.
 */
 
#ifndef CACHE_VIEW_H

	#define CACHE_VIEW_H
	
	/*!
	 * Message sent when the print slider is updated.
	 */
	#define PRINT_FCS_UPDATE_MSG 'pfum'
	
	/*!
	 * Message sent when the screen slider is updated.
	 */
	#define SCREEN_FCS_UPDATE_MSG 'sfum'
	
	/*!
	 * Message sent when the print slider is modified.
	 */
	#define PRINT_FCS_MODIFICATION_MSG 'pfmm'
	
	/*!
	 * Message sent when the screen slider is modified.
	 */
	#define SCREEN_FCS_MODIFICATION_MSG 'sfmm'
	
	#ifndef _VIEW_H
		
		#include <View.h>
	
	#endif
	
	#ifndef _BOX_H
	
		#include <Box.h>
		
	#endif
	#ifndef _SLIDER_H
	
		#include <Slider.h>
	 
	#endif
	#ifndef _STDIO_H
	
		#include <stdio.h>
		
	#endif
	#ifndef _BUTTON_H
	
		#include <Button.h>
		
	#endif
	
	class CacheView : public BView{
	
		public:
			
			CacheView(BRect frame, int minVal, int maxVal, int32 printCurrVal, int32 screenCurrVal);
			void updatePrintFCS(const char* txt);
			void updateScreenFCS(const char* txt);
			int getPrintFCSValue();
			int getScreenFCSValue();
			void revertToOriginal();
			void resetToDefaults();
			
		private:
		
			/**
			 * The screen cache slider.
			 */
			BSlider *screenFCS;
			
			/**
			 * The print cache slider.
			 */
			BSlider *printFCS;
			
			/**
			 * The original print slider value.
			 */
			int32 origPrintVal;
			
			/**
			 * The original screen slider value.
			 */
			int32 origScreenVal;
			
	};
	
#endif
