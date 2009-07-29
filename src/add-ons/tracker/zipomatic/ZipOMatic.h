#ifndef _ZIPOMATIC_H_
#define _ZIPOMATIC_H_


#include <Application.h>
#include <Message.h>


class ZipOMatic : public BApplication
{
public:
							ZipOMatic();
							~ZipOMatic();

	virtual	void			ReadyToRun();
	virtual	void			RefsReceived(BMessage* message);
	virtual	void			MessageReceived(BMessage* message);
	virtual	bool			QuitRequested();
	
private:
			void			_SilentRelaunch();
			void			_UseExistingOrCreateNewWindow(BMessage*
								message = NULL);

			bool			fGotRefs;
};

#endif // _ZIPOMATIC_H_

