/*! \file main.h
    \brief Header file for the main class.
    
*/

#ifndef MAIN_H
	
	#define MAIN_H
	
	#ifndef _APPLICATION_H
	
		#include <Application.h>
		
	#endif
	
	class FontsSettings;
	class MainWindow;
	/**
	 * Main class.
	 *
	 * Gets everything going.
	 */
	class Font_pref : public BApplication{
	
		public:
		
			/**
			 * Constructor.
			 */
			Font_pref();
			~Font_pref();
			void ReadyToRun();
			bool QuitRequested();
		private:
			FontsSettings *f_settings;
			MainWindow *window;
	};
	
#endif
