#ifndef _ZIPOMATIC_H_
#define _ZIPOMATIC_H_

#include <Application.h>
#include <Message.h>

class ZipOMatic : public BApplication
{
public:
	ZipOMatic (void);
	~ZipOMatic (void);

	virtual void	ReadyToRun		(void);
	virtual void	RefsReceived	(BMessage * a_message);
	virtual void	MessageReceived (BMessage * a_message);
	virtual bool	QuitRequested	(void);
	
	void			SilentRelaunch	(void);
	void			UseExistingOrCreateNewWindow  (BMessage * a_message  =  NULL);

private:

	bool			m_got_refs;
	
	
};

#endif // _ZIPOMATIC_H_
