#ifndef TRAY_VIEW
#define TRAY_VIEW

#include <InterfaceDefs.h>
#include <TranslationKit.h>
#include <Deskbar.h>
#include <OS.h>


#include "common.h"
#include "settings.h"
#include <Roster.h>

//exported instantiator function
extern "C" _EXPORT BView* instantiate_deskbar_item();


//since we can't remove the view from the deskbar from within the same thread
//as tray view, a thread will be spawned and this function called. It removed
//TrayView from the Deskbar
status_t removeFromDeskbar(void *);

class _EXPORT TrayView;


/*********************************************
	class TrayView derived from BView

	The icon in the Deskbar tray, provides the fundamental
	user interface. Archivable, so it can be flattened and
	fired at the deskbar.

*********************************************/

class TrayView:public BView{
	private:

		BBitmap *_activeIcon, *_inactiveIcon;
		bool watching;

		void _init(void); //initialization common to all constructors

	public:
		AutoRaiseSettings *_settings;
		mode_mouse fNormalMM;
		volatile int32 current_window; // id
		bigtime_t raise_delay;
		volatile thread_id last_raiser_thread;
		team_id fDeskbarTeam;

		bigtime_t polling_delay;
		sem_id fPollerSem;
		thread_id poller_thread;

		TrayView();
		TrayView(BMessage *mdArchive);
		virtual ~TrayView();

		virtual status_t Archive(BMessage *data, bool deep = true) const;
		static TrayView *Instantiate(BMessage *data);

		virtual void Draw(BRect updateRect );
		virtual void AttachedToWindow();
		virtual void MouseDown(BPoint where);
		virtual void MessageReceived(BMessage* message);
		virtual void GetPreferredSize(float *w, float *h);

		AutoRaiseSettings *Settings() const;
		void SetActive(bool);
};

int32 fronter(void *);
int32 poller(void *);

/*********************************************
	ConfigMenu derived from BPopUpMenu
	Provides the contextual left-click menu for the
	TrayView. Fires it's messages at the TrayView specified
	in it's constructor;
	Also, it's by default set to asynchronously destruct,
	so it's basically a fire & forget kinda fella.
*********************************************/

class ConfigMenu: public BPopUpMenu{
	private:

	public:
		ConfigMenu(TrayView *tv, bool useMag);
		virtual ~ConfigMenu();
};


#endif
