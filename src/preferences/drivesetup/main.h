/*! \file main.h
    \brief Header file for the main class.
    
*/

#ifndef MAIN_H
#define MAIN_H
	
	#include <Application.h>
	
	class PosSettings;
	
	
	/**
	 * Main class.
	 *
	 * Gets everything going.
	 */
	class DriveSetup : public BApplication
	{	
		public:
		
			/**
			 * Constructor.
			 */
			DriveSetup();
			virtual ~DriveSetup();
		
		private:
			PosSettings	*fSettings;
	};
	
#endif
