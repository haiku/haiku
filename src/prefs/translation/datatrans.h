/*
	
	datatrans.h

*/

#ifndef DATATRANS_H  //DATATRANS_H

#define DATATRANS_H

#ifndef _APPLICATION_H
#include <Application.h>
#endif

#ifndef LVIEW_H
#include "LView.h"
#endif

#include <String.h>

class DATApp : public BApplication 
{
public:
	DATApp();
	
	virtual void RefsReceived(BMessage *message);
	

private:
	
	BString string;	
	
};

#endif //DATATRANS_H