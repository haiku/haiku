/*
    OBOS ShowImage 0.1 - 17/02/2002 - 22:22 - Fernando Francisco de Oliveira
*/

#ifndef _ShowImageApp_h
#define _ShowImageApp_h

#include <Application.h>

class ShowImageApp : public BApplication
{
public:
	ShowImageApp();

public:
	virtual void AboutRequested();
	virtual void ArgvReceived(int32 argc, char** argv);
	virtual void MessageReceived(BMessage* message);
	virtual void ReadyToRun();
	virtual void Pulse();
	virtual void RefsReceived(BMessage* message);

private:
	void StartPulse();
	void Open(const entry_ref* ref);
		
private:
	BFilePanel*	m_pOpenPanel;
	bool fbpulseStarted;
};

#endif /* _ShowImageApp_h */
