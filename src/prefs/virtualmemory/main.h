/*! \file main.h
    \brief Header file for the main class.
    
*/

#ifndef MAIN_H
#define MAIN_H
	
#include <Application.h>
#include <Volume.h>
#include <VolumeRoster.h>
#include <stdio.h>
#include "VMSettings.h"
#include <OS.h>
#include <Entry.h>

class MainWindow; 
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
		virtual void ReadyToRun();
	private:
		VMSettings *fSettings;	
		MainWindow *window;	
	};
	
#endif
