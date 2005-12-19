#ifndef __ZIPPO_WINDOW_H__
#define __ZIPPO_WINDOW_H__

#include <Window.h>
#include <MenuBar.h>
#include <Bitmap.h>
#include <Menu.h>
#include <MenuItem.h>

#include "ZipOMaticSettings.h"
class ZippoView;
class ZipperThread;

class ZippoWindow : public BWindow
{
public:
					ZippoWindow			(BMessage * a_message = NULL);
					~ZippoWindow		(void);
	virtual void	MessageReceived 	(BMessage * a_message);
	virtual bool	QuitRequested		(void);
	virtual void	Zoom			 	(BPoint origin, float width, float height);
	
	bool			IsZipping			(void);
	
private:
			
	status_t		ReadSettings		(void);
	status_t		WriteSettings		(void);

	void			StartZipping		(BMessage * a_message);
	void			StopZipping			(void);
	
	void			CloseWindowOrKeepOpen (void);

	ZippoView	*	zippoview;
	ZippoSettings	m_zippo_settings;
	ZipperThread *	m_zipper_thread;
	
	bool	m_got_refs_at_window_startup;
	bool	m_zipping_was_stopped;
	
	BMessage	*	m_alert_invoker_message;
	BInvoker	*	m_alert_window_invoker;
};

#endif // __ZIPPO_WINDOW_H__
