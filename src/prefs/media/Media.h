/*

Media Header by Sikosis

(C)2003

*/

#ifndef __MEDIA_H__
#define __MEDIA_H__

extern const char *APP_SIGNATURE;

class Media : public BApplication
{
	public:
    	Media();
	    virtual void MessageReceived(BMessage *message);
	    
	private:
		
};

#endif