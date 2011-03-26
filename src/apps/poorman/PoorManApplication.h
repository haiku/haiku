/* PoorManApplication.h
 *
 *	Philip Harrison
 *	Started: 4/25/2004
 *	Version: 0.1
 */

#ifndef POOR_MAN_APPLICATION_H
#define POOR_MAN_APPLICATION_H


#include <Application.h>

class PoorManWindow;
//#include "PoorManPreferencesWindow.h"


class PoorManApplication: public BApplication
{
public:
					PoorManApplication();
	virtual void	MessageReceived(BMessage *message);
	PoorManWindow *	GetPoorManWindow() { return mainWindow; }
private:
			PoorManWindow				* mainWindow;
			//PoorManPreferencesWindow	* prefWindow;
			
			// --------------------------------------------
			// settings variables
			//bool	status;
			//char	directory[512];
			//uint32	hits;	
	
};

#endif
