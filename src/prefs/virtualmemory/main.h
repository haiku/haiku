/*! \file main.h
    \brief Header file for the main class.
    
*/

#ifndef MAIN_H
	
	#define MAIN_H
	
	#ifndef _APPLICATION_H
	
		#include <Application.h>
		
	#endif
	
	#ifndef _VOLUME_H
	
		#include <Volume.h>
		
	#endif
	#ifndef _VOLUME_ROSTER_H
	
		#include <VolumeRoster.h>
		
	#endif
	#ifndef _STDIO_H
	
		#include <stdio.h>
		
	#endif
	#ifndef VM_SETTINGS_H
	
		#include "VMSettings.h"
		
	#endif
	#ifndef _OS_H
	
		#include <OS.h>
		
	#endif
	#ifndef _ENTRY_H
	
		#include <Entry.h>
		
	#endif
	
	/**
	 * Main class.
	 *
	 * Gets everything going.
	 */
	class VM_pref : public BApplication{
	
		public:
		
			/**
			 * Constructor.
			 */
			VM_pref();
			
			/**
			 * Destructor.
			 */
			virtual ~VM_pref();
		
		private:
			VMSettings	*fSettings;
			
	};
	
#endif