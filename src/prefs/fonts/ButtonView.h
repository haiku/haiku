/*! \file ButtonView.h
 *  \brief Header file for the ButtonView class.
 */
 
#ifndef BUTTON_VIEW_H

	#define BUTTON_VIEW_H
	
	/*!
	 * Message sent when the rescan button is sent.
	 */
	#define RESCAN_FONTS_MSG 'rscn'
	
	/*!
	 * Message sent when the reset button is sent.
	 */
	#define RESET_FONTS_MSG 'rset'
	
	/*!
	 * Message sent when the revert button is sent.
	 */
	#define REVERT_MSG 'rvrt'
	
	#ifndef _VIEW_H
		
		#include <View.h>
	
	#endif
	
	#ifndef _BUTTON_H
	
		#include <Button.h>
		
	#endif
	
	class ButtonView : public BView{
	
		public:
			
			ButtonView(BRect frame);
			bool RevertState();
			void SetRevertState(bool b);
			
		private:
		
			/**
			 * The revert button.
			 */
			BButton *revertButton;
			
	};
	
#endif
