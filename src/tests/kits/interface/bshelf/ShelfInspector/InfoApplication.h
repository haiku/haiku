// TheApplication.h



#if !defined(THE_APPLICATION_H)
#define THE_APPLICATION_H


#include "IEApplication.h"
#include "InfoWindow.h"
#include <Roster.h>


IEResourceHandler *resourcehandler;

class InfoApplication : public IEApplication 
{
	public:
									InfoApplication();
									~InfoApplication();
		
		virtual void 	MessageReceived(BMessage *msg);
		virtual void 	ReadyToRun(void);
		
	private:
		BMessenger	fMsgnr;	
		InfoWindow  *fInfoWindow;
		bool				fLaunched;
};

#endif 
