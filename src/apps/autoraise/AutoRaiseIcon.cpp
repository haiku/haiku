/* this is for the DANO hack (new, simpler version than the BStringIO hack) */
#include <BeBuild.h>
#ifdef B_BEOS_VERSION_DANO
#define private public
#include <Messenger.h>
#undef private
#endif

#include "AutoRaiseIcon.h"
#include <stdlib.h>
#include <DataIO.h>
#include <Screen.h>
#include <View.h>
#include <Debug.h>


extern "C" _EXPORT BView *instantiate_deskbar_item(void)
{
	puts("Instanciating AutoRaise TrayView...");
	return (new TrayView);
}


long removeFromDeskbar(void *)
{
	BDeskbar db;
	if (db.RemoveItem(APP_NAME) != B_OK)
		printf("Unable to remove AutoRaise from BDeskbar\n");

	return 0;
}

//**************************************************

ConfigMenu::ConfigMenu(TrayView *tv, bool useMag)
	:BPopUpMenu("config_popup", false, false){

	BMenu *tmpm;
	BMenuItem *tmpi;
	BMessage *msg;

	AutoRaiseSettings *s = tv->Settings();

	SetFont(be_plain_font);


	BMenuItem *active = new BMenuItem("Active", new BMessage(MSG_TOOGLE_ACTIVE));
	active->SetMarked(s->Active());
	AddItem(active);

	tmpm = new BMenu("Mode");
	tmpm->SetFont(be_plain_font);

	msg = new BMessage(MSG_SET_MODE);
	msg->AddInt32(AR_MODE, Mode_All);
	tmpi = new BMenuItem("Default (all windows)", msg);
	tmpi->SetMarked(s->Mode() == Mode_All);
	tmpm->AddItem(tmpi);

	msg = new BMessage(MSG_SET_MODE);
	msg->AddInt32(AR_MODE, Mode_DeskbarOver);
	tmpi = new BMenuItem("Deskbar only (over its area)", msg);
	tmpi->SetMarked(s->Mode() == Mode_DeskbarOver);
#ifdef USE_DANO_HACK
	tmpi->SetEnabled(false);
#endif
	tmpm->AddItem(tmpi);

	msg = new BMessage(MSG_SET_MODE);
	msg->AddInt32(AR_MODE, Mode_DeskbarTouch);
	tmpi = new BMenuItem("Deskbar only (touch)", msg);
	tmpi->SetMarked(s->Mode() == Mode_DeskbarTouch);
	tmpm->AddItem(tmpi);


	tmpm->SetTargetForItems(tv);
	BMenuItem *modem = new BMenuItem(tmpm);
	modem->SetEnabled(s->Active());
	AddItem(modem);

	tmpm = new BMenu("Inactive behaviour");
	tmpm->SetFont(be_plain_font);

	msg = new BMessage(MSG_SET_BEHAVIOUR);
	msg->AddInt32(AR_BEHAVIOUR, B_NORMAL_MOUSE);
	tmpi = new BMenuItem("Normal", msg);
	tmpi->SetMarked(tv->fNormalMM == B_NORMAL_MOUSE);
	tmpm->AddItem(tmpi);

	msg = new BMessage(MSG_SET_BEHAVIOUR);
	msg->AddInt32(AR_BEHAVIOUR, B_FOCUS_FOLLOWS_MOUSE);
	tmpi = new BMenuItem("Focus follows mouse", msg);
	tmpi->SetMarked(tv->fNormalMM == B_FOCUS_FOLLOWS_MOUSE);
	tmpm->AddItem(tmpi);

	msg = new BMessage(MSG_SET_BEHAVIOUR);
	msg->AddInt32(AR_BEHAVIOUR, B_WARP_FOCUS_FOLLOWS_MOUSE);
	tmpi = new BMenuItem("Warping (ffm)", msg);
	tmpi->SetMarked(tv->fNormalMM == (mode_mouse)B_WARP_FOCUS_FOLLOWS_MOUSE);
	tmpm->AddItem(tmpi);

	msg = new BMessage(MSG_SET_BEHAVIOUR);
	msg->AddInt32(AR_BEHAVIOUR, B_INSTANT_WARP_FOCUS_FOLLOWS_MOUSE);
	tmpi = new BMenuItem("Instant warping (ffm)", msg);
	tmpi->SetMarked(tv->fNormalMM == (mode_mouse)B_INSTANT_WARP_FOCUS_FOLLOWS_MOUSE);
	tmpm->AddItem(tmpi);

	tmpm->SetTargetForItems(tv);
	BMenuItem *behavm = new BMenuItem(tmpm);
	AddItem(behavm);


	tmpm = new BMenu("Delay");
	tmpm->SetFont(be_plain_font);

	msg = new BMessage(MSG_SET_DELAY);
	msg->AddInt64(AR_DELAY, 100000LL);
	tmpi = new BMenuItem("0.1 s", msg);
	tmpi->SetMarked(tv->raise_delay == 100000LL);
	tmpm->AddItem(tmpi);

	msg = new BMessage(MSG_SET_DELAY);
	msg->AddInt64(AR_DELAY, 200000LL);
	tmpi = new BMenuItem("0.2 s", msg);
	tmpi->SetMarked(tv->raise_delay == 200000LL);
	tmpm->AddItem(tmpi);

	msg = new BMessage(MSG_SET_DELAY);
	msg->AddInt64(AR_DELAY, 500000LL);
	tmpi = new BMenuItem("0.5 s", msg);
	tmpi->SetMarked(tv->raise_delay == 500000LL);
	tmpm->AddItem(tmpi);

	msg = new BMessage(MSG_SET_DELAY);
	msg->AddInt64(AR_DELAY, 1000000LL);
	tmpi = new BMenuItem("1 s", msg);
	tmpi->SetMarked(tv->raise_delay == 1000000LL);
	tmpm->AddItem(tmpi);

	msg = new BMessage(MSG_SET_DELAY);
	msg->AddInt64(AR_DELAY, 2000000LL);
	tmpi = new BMenuItem("2 s", msg);
	tmpi->SetMarked(tv->raise_delay == 2000000LL);
	tmpm->AddItem(tmpi);

	msg = new BMessage(MSG_SET_DELAY);
	msg->AddInt64(AR_DELAY, 3000000LL);
	tmpi = new BMenuItem("3 s", msg);
	tmpi->SetMarked(tv->raise_delay == 3000000LL);
	tmpm->AddItem(tmpi);

	msg = new BMessage(MSG_SET_DELAY);
	msg->AddInt64(AR_DELAY, 4000000LL);
	tmpi = new BMenuItem("4 s", msg);
	tmpi->SetMarked(tv->raise_delay == 4000000LL);
	tmpm->AddItem(tmpi);

	msg = new BMessage(MSG_SET_DELAY);
	msg->AddInt64(AR_DELAY, 5000000LL);
	tmpi = new BMenuItem("5 s", msg);
	tmpi->SetMarked(tv->raise_delay == 5000000LL);
	tmpm->AddItem(tmpi);

	tmpm->SetTargetForItems(tv);
	BMenuItem *delaym = new BMenuItem(tmpm);
	delaym->SetEnabled(s->Active());

	AddItem(delaym);

	AddSeparatorItem();
//	AddItem(new BMenuItem("Settings...", new BMessage(OPEN_SETTINGS)));

	AddItem(new BMenuItem("About "APP_NAME B_UTF8_ELLIPSIS,
		new BMessage(B_ABOUT_REQUESTED)));
	AddItem(new BMenuItem("Remove from tray", new BMessage(REMOVE_FROM_TRAY)));

	SetTargetForItems(tv);
	SetAsyncAutoDestruct(true);
}

ConfigMenu::~ConfigMenu() {}

//************************************************

TrayView::TrayView()
	:BView(BRect(0, 0, B_MINI_ICON, B_MINI_ICON -1), "AutoRaise", B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW){
	_init(); 	//Initialization common to both constructors
}

//Rehydratable constructor
TrayView::TrayView(BMessage *mdArchive):BView(mdArchive){
	_init();		//As above
}

void TrayView::GetPreferredSize(float *w, float *h)
{
	*w = B_MINI_ICON;
	*h = B_MINI_ICON - 1;
}

void TrayView::_init()
{
	thread_info ti;
	status_t err;

	watching = false;
	_settings = new AutoRaiseSettings;

	_appPath = _settings->AppPath();

	raise_delay = _settings->Delay();
	current_window = 0;
	polling_delay = 100000;
	fPollerSem = create_sem(0, "AutoRaise poller sync");
	last_raiser_thread = 0;
	fNormalMM = mouse_mode();

	_activeIcon = NULL;
	_inactiveIcon = NULL;

	get_thread_info(find_thread(NULL), &ti);
	fDeskbarTeam = ti.team;

#ifndef USE_DANO_HACK
	resume_thread(poller_thread = spawn_thread(poller, "AutoRaise desktop "
		"poller", B_NORMAL_PRIORITY, (void *)this));
#endif

	//determine paths to icon files based on app path in settings file

	BResources res;
	BFile theapp(&_appPath, B_READ_ONLY);
	if ((err = res.SetTo(&theapp)) != B_OK) {

		printf("Unable to find the app to get the resources !!!\n");
//		removeFromDeskbar(NULL);
//		delete _settings;
//		return;
	}

	size_t bmsz;
	char *p;

	p = (char *)res.LoadResource('MICN', ACTIVE_ICON, &bmsz);
	_activeIcon = new BBitmap(BRect(0, 0, B_MINI_ICON-1, B_MINI_ICON -1),
		B_CMAP8);
	if (!p)
		puts("ERROR loading active icon");
	else
		_activeIcon->SetBits(p, B_MINI_ICON*B_MINI_ICON, 0, B_CMAP8);

	p = (char *)res.LoadResource('MICN', INACTIVE_ICON, &bmsz);
	_inactiveIcon = new BBitmap(BRect(0, 0, B_MINI_ICON-1, B_MINI_ICON -1),
		B_CMAP8);
	if (!p)
		puts("ERROR loading inactive icon");
	else
		_inactiveIcon->SetBits(p, B_MINI_ICON*B_MINI_ICON, 0, B_CMAP8);


	SetDrawingMode(B_OP_ALPHA);
	SetFlags(Flags() | B_WILL_DRAW);

	// begin watching if we want
	// (doesn't work here, better do it in AttachedToWindow())
}

TrayView::~TrayView(){
	status_t ret;

	if (watching) {
#ifdef USE_DANO_HACK
		be_roster->StopWatching(this);
#else
//		acquire_sem(fPollerSem);
#endif
		set_mouse_mode(fNormalMM);
		watching = false;
	}
	delete_sem(fPollerSem);
#ifndef USE_DANO_HACK
	wait_for_thread(poller_thread, &ret);
#endif
	if (_activeIcon) delete _activeIcon;
	if (_inactiveIcon) delete _inactiveIcon;
	if (_settings) delete _settings;

	return;
}

//Dehydrate into a message (called by the DeskBar)
status_t TrayView::Archive(BMessage *data, bool deep) const {
//	BEntry appentry(&_appPath, true);
//	BPath appPath(&appentry);
	status_t error=BView::Archive(data, deep);
	data->AddString("add_on", APP_SIG);
//	data->AddFlat("_appPath", (BFlattenable *) &_appPath);
	data->AddRef("_appPath", &_appPath);

	return error;
}

//Rehydrate the View from a given message (called by the DeskBar)
TrayView *TrayView::Instantiate(BMessage *data) {

	if (!validate_instantiation(data, "TrayView"))
	{
		return NULL;
	}

	return (new TrayView(data));
}

void TrayView::AttachedToWindow() {
	if(Parent())
		SetViewColor(Parent()->ViewColor());
		if (_settings->Active()) {
			fNormalMM = mouse_mode();
			set_mouse_mode(B_FOCUS_FOLLOWS_MOUSE);
#ifdef USE_DANO_HACK
			be_roster->StartWatching(this, B_REQUEST_WINDOW_ACTIVATED);
#else
			release_sem(fPollerSem);
#endif
			watching = true;
		}
}

void TrayView::Draw(BRect updaterect) {
	BRect bnds(Bounds());

	if (Parent()) SetHighColor(Parent()->ViewColor());
	else SetHighColor(189, 186, 189, 255);
	FillRect(bnds);

	if (_settings->Active())
	{
		if (_activeIcon) DrawBitmap(_activeIcon);
	}
	else
	{
		if (_inactiveIcon) DrawBitmap(_inactiveIcon);
	}
}

void TrayView::MouseDown(BPoint where) {
	BWindow *window = Window();	/*To handle the MouseDown message*/
	if (!window)	/*Check for proper instantiation*/
		return;

	BMessage *mouseMsg = window->CurrentMessage();
	if (!mouseMsg)	/*Check for existence*/
		return;

	if (mouseMsg->what == B_MOUSE_DOWN) {
		/*Variables for storing the button pressed / modifying key*/
		uint32 	buttons = 0;
		uint32  modifiers = 0;

		/*Get the button pressed*/
		mouseMsg->FindInt32("buttons", (int32 *) &buttons);
		/*Get modifier key (if any)*/
		mouseMsg->FindInt32("modifiers", (int32 *) &modifiers);

		/*Now perform action*/
		switch(buttons) {
			case B_PRIMARY_MOUSE_BUTTON:
			{
				SetActive(!_settings->Active());

				break;
			}
			case B_SECONDARY_MOUSE_BUTTON:
			{
				ConvertToScreen(&where);

				//menu will delete itself (see constructor of ConfigMenu),
				//so all we're concerned about is calling Go() asynchronously
				ConfigMenu *menu = new ConfigMenu(this, false);
				menu->Go(where, true, true, ConvertToScreen(Bounds()), true);

				break;
			}
		}
	}
}


int32 fronter(void *arg)
{
	TrayView *tv = (TrayView *)arg;
	int32 tok = tv->current_window;
	int32 ws = current_workspace();
	sem_id sem = tv->fPollerSem;

	snooze(tv->raise_delay);

#ifndef USE_DANO_HACK
	if (acquire_sem(sem) != B_OK)
		return B_OK; // this really needs a better locking model...
#endif
	if (ws != current_workspace())
		goto end; // don't touch windows if we changed workspace
	if (tv->last_raiser_thread != find_thread(NULL))
		goto end; // seems a newer one has been spawn, exit
PRINT(("tok = %ld cw = %ld\n", tok, tv->current_window));
	if (tok == tv->current_window) {
		bool doZoom = false;
		BRect zoomRect(0.0f, 0.0f, 10.0f, 10.0f);
		do_window_action(tok, B_BRING_TO_FRONT, zoomRect, doZoom);
	}

	end:
	release_sem(sem);
	return B_OK;
}

#ifndef USE_DANO_HACK

int32 poller(void *arg)
{
	TrayView *tv = (TrayView *)arg;
	volatile int32 tok = tv->current_window;
	int32 *tl = NULL;
	int32 i, tlc;
	window_info *wi = NULL;

	int pass=0;
	BPoint mouse;
	uint32 buttons;

	while (acquire_sem(tv->fPollerSem) == B_OK) {
		release_sem(tv->fPollerSem);
		pass++;
		BLooper *l = tv->Looper();
		if (!l || l->LockWithTimeout(500000) != B_OK)
			continue;
		tv->GetMouse(&mouse, &buttons);
		tv->ConvertToScreen(&mouse);
		tv->Looper()->Unlock();
		if (buttons) // we don't want to interfere when the user is moving a window or something...
			goto zzz;

		tl = get_token_list(-1, &tlc);
		for (i=0; i<tlc; i++) {
			wi = get_window_info(tl[i]);
			if (wi) {
				if (wi->layer < 3) // we hit the desktop or a window not on this WS
					goto zzz;
				if ((wi->window_left > wi->window_right) || (wi->window_top > wi->window_bottom))
					goto zzz; // invalid window ?
/*
printf("if (!%s && (%li, %li)isin(%li)(%li, %li, %li, %li) && (%li != %li) ", wi->is_mini?"true":"false",
	(long)mouse.x, (long)mouse.y, i, wi->window_left, wi->window_right, wi->window_top, wi->window_bottom, wi->id, tok);
*/


				if ((!wi->is_mini)
						&& (((long)mouse.x) > wi->window_left) && (((long)mouse.x) < wi->window_right)
						&& (((long)mouse.y) > wi->window_top) && (((long)mouse.y) < wi->window_bottom)) {
//((tv->_settings->Mode() != Mode_DeskbarOver) || (wi->team == tv->fDeskbarTeam))

					if ((tv->_settings->Mode() == Mode_All) && (wi->id == tv->current_window))
						goto zzz; // already raised

					if ((tv->_settings->Mode() == Mode_All) || (wi->team == tv->fDeskbarTeam)) {
						tv->current_window = wi->id;
						tok = wi->id;
						resume_thread(tv->last_raiser_thread = spawn_thread(fronter, "fronter", B_NORMAL_PRIORITY, (void *)tv));
						goto zzz;
					} else if (tv->_settings->Mode() == Mode_DeskbarTouch) // give up, before we find Deskbar under it
						goto zzz;
				}
				free(wi);
				wi=NULL;
			} else
				goto zzz;
		}
	zzz:
//		puts("");
		if (wi) free(wi);
		wi = NULL;
		if (tl) free(tl);
		tl = NULL;
		snooze(tv->polling_delay);
	}
	return B_OK;
}

#endif

void TrayView::MessageReceived(BMessage* message)
{
	BMessenger msgr;

	BAlert *alert;
	bigtime_t delay;
	int32 mode;

	switch(message->what)
	{
		case MSG_TOOGLE_ACTIVE:
			SetActive(!_settings->Active());
			break;
		case MSG_SET_ACTIVE:
			SetActive(true);
			break;
		case MSG_SET_INACTIVE:
			SetActive(false);
			break;
		case MSG_SET_DELAY:
			delay = DEFAULT_DELAY;
			message->FindInt64(AR_DELAY, &delay);
			raise_delay = delay;
			_settings->SetDelay(delay);
			break;
		case MSG_SET_MODE:
			mode = Mode_All;
			message->FindInt32(AR_MODE, &mode);
			_settings->SetMode(mode);
			break;
		case MSG_SET_BEHAVIOUR:
		{
			message->FindInt32(AR_BEHAVIOUR, &mode);
			bool wasactive = _settings->Active();
			if (wasactive)
				SetActive(false);
			fNormalMM = (mode_mouse)mode;
			set_mouse_mode(fNormalMM);
			if (wasactive)
				SetActive(true);
			break;
		}
		case REMOVE_FROM_TRAY:
		{
			thread_id tid = spawn_thread(removeFromDeskbar, "RemoveFromDeskbar", B_NORMAL_PRIORITY, NULL);
			if (tid) resume_thread(tid);

			break;
		}
		case B_ABOUT_REQUESTED:
			alert = new BAlert("about box", "AutoRaise, (c) 2002, mmu_man\nEnjoy :-)", "OK", NULL, NULL,
                B_WIDTH_AS_USUAL, B_OFFSET_SPACING, B_INFO_ALERT);
	        alert->SetShortcut(0, B_ENTER);
	        alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
    	    alert->Go(NULL); // use asynchronous version
			break;
		case OPEN_SETTINGS:

			break;
#ifdef USE_DANO_HACK
		case B_SOME_WINDOW_ACTIVATED:
//			printf("Window Activated\n");
//			message->PrintToStream();
			BPoint mouse;
			uint32 buttons;
			GetMouse(&mouse, &buttons);
			if (buttons)
				break;
			if (message->FindMessenger("be:window", &msgr) < B_OK)
				puts("BMsgr ERROR");
			else {
				bool doZoom = false;
                BRect zoomRect(0.0f, 0.0f, 0.0f, 0.0f);
				port_id pi = msgr.fPort;
//				printf("port:%li (%lx)\n", pi, pi);

				int32 *tl, tlc;
				tl = get_token_list(msgr.Team(), &tlc);
//				printf("tokens (team %li): (%li) ", msgr.Team(), tlc);
				for (tlc; tlc; tlc--) {
//					printf("%li ", tl[tlc-1]);
					window_info *wi = get_window_info(tl[tlc-1]);
					if (wi) {
						if (wi->client_port == pi) {
							if ((wi->layer < 3) // we hit the desktop or a window not on this WS
									|| (wi->window_left > wi->window_right) || (wi->window_top > wi->window_bottom)
									|| (wi->is_mini)
									|| (/*(_settings->Mode() == Mode_All) && */(wi->id == current_window))) {
								// already raised
								free(wi);
								break;
							}

							if ((_settings->Mode() == Mode_All) || (wi->team == fDeskbarTeam)) {
								PRINT(("raising wi=%li, cp=%ld, pi=%ld team=%ld DBteam=%ld\n", wi->id, wi->client_port, pi, wi->team, fDeskbarTeam));
								current_window = wi->id;
								int32 tok = wi->id;
								resume_thread(last_raiser_thread = spawn_thread(fronter, "fronter", B_NORMAL_PRIORITY, (void *)this));
							} else {
								current_window = wi->id;
							}
						}
						free(wi);
					}
				}
//				puts("");
				free(tl);
			}
			break;
#endif
		default:
			BView::MessageReceived(message);
	}
}

AutoRaiseSettings *TrayView::Settings() const
{
	return _settings;
}

void TrayView::SetActive(bool st)
{
	_settings->SetActive(st);
	if (_settings->Active())
	{
		if (!watching) {
			fNormalMM = mouse_mode();
			set_mouse_mode(B_FOCUS_FOLLOWS_MOUSE);
#ifdef USE_DANO_HACK
			be_roster->StartWatching(this, B_REQUEST_WINDOW_ACTIVATED);
#else
			release_sem(fPollerSem);
#endif
			watching = true;
		}
	}
	else
	{
		if (watching) {
#ifdef USE_DANO_HACK
			be_roster->StopWatching(this);
#else
			acquire_sem(fPollerSem);
#endif
			set_mouse_mode(fNormalMM);
			watching = false;
		}
	}
	Invalidate();
}

