/*

Devices Header by Sikosis

(C)2003 OBOS

*/

#ifndef __DEVICES_H__
#define __DEVICES_H__

extern const char *APP_SIGNATURE;

class Devices : public BApplication
{
	public:
    	Devices();
	    virtual void MessageReceived(BMessage *message);
	    
	private:
		
};

#endif
