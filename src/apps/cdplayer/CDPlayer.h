/*

CDPlayer Header

Author: Sikosis

(C)2004 Haiku - http://haiku-os.org/

*/

#ifndef __CDPLAYER_H__
#define __CDPLAYER_H__

extern const char *APP_SIGNATURE;

class CDPlayer : public BApplication
{
	public:
		CDPlayer();
		virtual void MessageReceived(BMessage *message);
	private:

};

#endif
