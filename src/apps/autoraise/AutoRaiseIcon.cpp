//#define DEBUG 1
#include <BeBuild.h>

#include "AutoRaiseIcon.h"
#include <stdlib.h>
#include <Catalog.h>
#include <DataIO.h>
#include <Screen.h>
#include <View.h>
#include <Debug.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "AutoRaiseIcon"


extern "C" _EXPORT BView *instantiate_deskbar_item(void)
{
	puts("Instanciating AutoRaise TrayView...");
	return (new TrayView);
}


status_t removeFromDeskbar(void *)
{
	BDeskbar db;
	if (db.RemoveItem(APP_NAME) != B_OK) {
		printf("Unable to remove AutoRaise from BDeskbar\n");
	}

	return B_OK;
}


static status_t
our_image(image_info& image)
{
	int32 cookie = 0;
	while (get_next_image_info(B_CURRENT_TEAM, &cookie, &image) == B_OK) {
		if ((char *)our_image >= (char *)image.text
			&& (char *)our_image <= (char *)image.text + image.text_size)
			return B_OK;
	}

	return B_ERROR;
}


//**************************************************

ConfigMenu::ConfigMenu(TrayView *tv, bool useMag)
	:BPopUpMenu("config_popup", false, false){

	BMenu *tmpm;
	BMenuItem *tmpi;
	BMessage *msg;

	AutoRaiseSettings *s = tv->Settings();

	SetFont(be_plain_font);


	BMenuItem *active = new BMenuItem(B_TRANSLATE("Active"),
		new BMessage(MSG_TOGGLE_ACTIVE));
	active->SetMarked(s->Active());
	AddItem(active);

	tmpm = new BMenu(B_TRANSLATE("Mode"));
	tmpm->SetFont(be_plain_font);

	msg = new BMessage(MSG_SET_MODE);
	msg->AddInt32(AR_MODE, Mode_All);
	tmpi = new BMenuItem(B_TRANSLATE("Default (all windows)"), msg);
	tmpi->SetMarked(s->Mode() == Mode_All);
	tmpm->AddItem(tmpi);

	msg = new BMessage(MSG_SET_MODE);
	msg->AddInt32(AR_MODE, Mode_DeskbarOver);
	tmpi = new BMenuItem(B_TRANSLATE("Deskbar only (over its area)"), msg);
	tmpi->SetMarked(s->Mode() == Mode_DeskbarOver);
	tmpm->AddItem(tmpi);

	msg = new BMessage(MSG_SET_MODE);
	msg->AddInt32(AR_MODE, Mode_DeskbarTouch);
	tmpi = new BMenuItem(B_TRANSLATE("Deskbar only (touch)"), msg);
	tmpi->SetMarked(s->Mode() == Mode_DeskbarTouch);
	tmpm->AddItem(tmpi);


	tmpm->SetTargetForItems(tv);
	BMenuItem *modem = new BMenuItem(tmpm);
	modem->SetEnabled(s->Active());
	AddItem(modem);

	tmpm = new BMenu(B_TRANSLATE("Inactive behaviour"));
	tmpm->SetFont(be_plain_font);

	msg = new BMessage(MSG_SET_BEHAVIOUR);
	msg->AddInt32(AR_BEHAVIOUR, B_NORMAL_MOUSE);
	tmpi = new BMenuItem(B_TRANSLATE("Normal"), msg);
	tmpi->SetMarked(tv->fNormalMM == B_NORMAL_MOUSE);
	tmpm->AddItem(tmpi);

	msg = new BMessage(MSG_SET_BEHAVIOUR);
	msg->AddInt32(AR_BEHAVIOUR, B_FOCUS_FOLLOWS_MOUSE);
	tmpi = new BMenuItem(B_TRANSLATE("Focus follows mouse"), msg);
	tmpi->SetMarked(tv->fNormalMM == B_FOCUS_FOLLOWS_MOUSE);
	tmpm->AddItem(tmpi);

	msg = new BMessage(MSG_SET_BEHAVIOUR);
	msg->AddInt32(AR_BEHAVIOUR, B_WARP_FOCUS_FOLLOWS_MOUSE);
	tmpi = new BMenuItem(B_TRANSLATE("Warping (ffm)"), msg);
	tmpi->SetMarked(tv->fNormalMM == (mode_mouse)B_WARP_FOCUS_FOLLOWS_MOUSE);
	tmpm->AddItem(tmpi);

	msg = new BMessage(MSG_SET_BEHAVIOUR);
	msg->AddInt32(AR_BEHAVIOUR, B_INSTANT_WARP_FOCUS_FOLLOWS_MOUSE);
	tmpi = new BMenuItem(B_TRANSLATE("Instant warping (ffm)"), msg);
	tmpi->SetMarked(tv->fNormalMM == (mode_mouse)B_INSTANT_WARP_FOCUS_FOLLOWS_MOUSE);
	tmpm->AddItem(tmpi);

	tmpm->SetTargetForItems(tv);
	BMenuItem *behavm = new BMenuItem(tmpm);
	AddItem(behavm);


	tmpm = new BMenu(B_TRANSLATE("Delay"));
	tmpm->SetFont(be_plain_font);

	msg = new BMessage(MSG_SET_DELAY);
	msg->AddInt64(AR_DELAY, 100000LL);
	tmpi = new BMenuItem(B_TRANSLATE("0.1 s"), msg);
	tmpi->SetMarked(tv->raise_delay == 100000LL);
	tmpm->AddItem(tmpi);

	msg = new BMessage(MSG_SET_DELAY);
	msg->AddInt64(AR_DELAY, 200000LL);
	tmpi = new BMenuItem(B_TRANSLATE("0.2 s"), msg);
	tmpi->SetMarked(tv->raise_delay == 200000LL);
	tmpm->AddItem(tmpi);

	msg = new BMessage(MSG_SET_DELAY);
	msg->AddInt64(AR_DELAY, 500000LL);
	tmpi = new BMenuItem(B_TRANSLATE("0.5 s"), msg);
	tmpi->SetMarked(tv->raise_delay == 500000LL);
	tmpm->AddItem(tmpi);

	msg = new BMessage(MSG_SET_DELAY);
	msg->AddInt64(AR_DELAY, 1000000LL);
	tmpi = new BMenuItem(B_TRANSLATE("1 s"), msg);
	tmpi->SetMarked(tv->raise_delay == 1000000LL);
	tmpm->AddItem(tmpi);

	msg = new BMessage(MSG_SET_DELAY);
	msg->AddInt64(AR_DELAY, 2000000LL);
	tmpi = new BMenuItem(B_TRANSLATE("2 s"), msg);
	tmpi->SetMarked(tv->raise_delay == 2000000LL);
	tmpm->AddItem(tmpi);

	msg = new BMessage(MSG_SET_DELAY);
	msg->AddInt64(AR_DELAY, 3000000LL);
	tmpi = new BMenuItem(B_TRANSLATE("3 s"), msg);
	tmpi->SetMarked(tv->raise_delay == 3000000LL);
	tmpm->AddItem(tmpi);

	msg = new BMessage(MSG_SET_DELAY);
	msg->AddInt64(AR_DELAY, 4000000LL);
	tmpi = new BMenuItem(B_TRANSLATE("4 s"), msg);
	tmpi->SetMarked(tv->raise_delay == 4000000LL);
	tmpm->AddItem(tmpi);

	msg = new BMessage(MSG_SET_DELAY);
	msg->AddInt64(AR_DELAY, 5000000LL);
	tmpi = new BMenuItem(B_TRANSLATE("5 s"), msg);
	tmpi->SetMarked(tv->raise_delay == 5000000LL);
	tmpm->AddItem(tmpi);

	tmpm->SetTargetForItems(tv);
	BMenuItem *delaym = new BMenuItem(tmpm);
	delaym->SetEnabled(s->Active());

	AddItem(delaym);

	AddSeparatorItem();
//	AddItem(new BMenuItem("Settings...", new BMessage(OPEN_SETTINGS)));

	AddItem(new BMenuItem(B_TRANSLATE("About " APP_NAME B_UTF8_ELLIPSIS),
		new BMessage(B_ABOUT_REQUESTED)));
	AddItem(new BMenuItem(B_TRANSLATE("Remove from tray"),
		new BMessage(REMOVE_FROM_TRAY)));

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

	watching = false;
	_settings = new AutoRaiseSettings;

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

	resume_thread(poller_thread = spawn_thread(poller, "AutoRaise desktop "
		"poller", B_NORMAL_PRIORITY, (void *)this));

	image_info info;
	{
		status_t result = our_image(info);
		if (result != B_OK) {
			printf("Unable to lookup image_info for the AutoRaise image: %s\n",
				strerror(result));
			removeFromDeskbar(NULL);
			return;
		}
	}

	BFile file(info.name, B_READ_ONLY);
	if (file.InitCheck() != B_OK) {
		printf("Unable to access AutoRaise image file: %s\n",
			strerror(file.InitCheck()));
		removeFromDeskbar(NULL);
		return;
	}

	BResources res(&file);
	if (res.InitCheck() != B_OK) {
		printf("Unable to load image resources: %s\n",
			strerror(res.InitCheck()));
		removeFromDeskbar(NULL);
		return;
	}

	size_t bmsz;
	char *p;

	p = (char *)res.LoadResource('MICN', ACTIVE_ICON, &bmsz);
	_activeIcon = new BBitmap(BRect(0, 0, B_MINI_ICON-1, B_MINI_ICON -1),
		B_CMAP8);
	if (p == NULL) {
		puts("ERROR loading active icon");
		removeFromDeskbar(NULL);
		return;
	}
	_activeIcon->SetBits(p, B_MINI_ICON * B_MINI_ICON, 0, B_CMAP8);

	p = (char *)res.LoadResource('MICN', INACTIVE_ICON, &bmsz);
	_inactiveIcon = new BBitmap(BRect(0, 0, B_MINI_ICON-1, B_MINI_ICON -1),
		B_CMAP8);
	if (p == NULL) {
		puts("ERROR loading inactive icon");
		removeFromDeskbar(NULL);
		return;
	}
	_inactiveIcon->SetBits(p, B_MINI_ICON * B_MINI_ICON, 0, B_CMAP8);

	SetDrawingMode(B_OP_ALPHA);
	SetFlags(Flags() | B_WILL_DRAW);

	// begin watching if we want
	// (doesn't work here, better do it in AttachedToWindow())
}

TrayView::~TrayView(){
	status_t ret;

	if (watching) {
		set_mouse_mode(fNormalMM);
		watching = false;
	}
	delete_sem(fPollerSem);
	wait_for_thread(poller_thread, &ret);
	if (_activeIcon) delete _activeIcon;
	if (_inactiveIcon) delete _inactiveIcon;
	if (_settings) delete _settings;

	return;
}

//Dehydrate into a message (called by the DeskBar)
status_t TrayView::Archive(BMessage *data, bool deep) const {
	status_t error=BView::Archive(data, deep);
	data->AddString("add_on", APP_SIG);

	return error;
}

//Rehydrate the View from a given message (called by the DeskBar)
TrayView *TrayView::Instantiate(BMessage *data) {

	if (!validate_instantiation(data, "TrayView")) {
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
		release_sem(fPollerSem);
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
	int32 ws = current_workspace();
	volatile int32 tok = tv->current_window;
	sem_id sem = tv->fPollerSem;

	snooze(tv->raise_delay);

	if (acquire_sem(sem) != B_OK)
		return B_OK; // this really needs a better locking model...
	if (ws != current_workspace())
		goto end; // don't touch windows if we changed workspace
	if (tv->last_raiser_thread != find_thread(NULL))
		goto end; // seems a newer one has been spawn, exit
PRINT(("tok = %" B_PRId32 " cw = %" B_PRId32 "\n", tok, tv->current_window));
	if (tok == tv->current_window) {
		bool doZoom = false;
		BRect zoomRect(0.0f, 0.0f, 10.0f, 10.0f);
		do_window_action(tok, B_BRING_TO_FRONT, zoomRect, doZoom);
	}

	end:
	release_sem(sem);
	return B_OK;
}


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
			free(wi);
			wi = get_window_info(tl[i]);
			if (wi) {
PRINT(("wi [%" B_PRId32 "] = %p, %" B_PRId32 " %s\n", i, wi, wi->layer,
	((struct client_window_info *)wi)->name));
				if (wi->layer < 3) // we hit the desktop or a window not on this WS
					continue;
				if ((wi->window_left > wi->window_right) || (wi->window_top > wi->window_bottom))
					continue; // invalid window ?
				if (wi->is_mini)
					continue;

PRINT(("if (!%s && (%li, %li)isin(%" B_PRId32 ")(%" B_PRId32 ", %" B_PRId32
	", %" B_PRId32 ", %" B_PRId32 ") && (%" B_PRId32 " != %" B_PRId32 ") \n",
	wi->is_mini?"true":"false", (long)mouse.x, (long)mouse.y, i,
	wi->window_left, wi->window_right, wi->window_top, wi->window_bottom,
	wi->server_token, tok));



				if ((((long)mouse.x) > wi->window_left) && (((long)mouse.x) < wi->window_right)
					&& (((long)mouse.y) > wi->window_top) && (((long)mouse.y) < wi->window_bottom)) {
//((tv->_settings->Mode() != Mode_DeskbarOver) || (wi->team == tv->fDeskbarTeam))

					if ((tv->_settings->Mode() == Mode_All) && 
					(wi->server_token == tv->current_window))
						goto zzz; // already raised

					if ((tv->_settings->Mode() == Mode_All) || (wi->team == tv->fDeskbarTeam)) {
						tv->current_window = wi->server_token;
						tok = wi->server_token;
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


void TrayView::MessageReceived(BMessage* message)
{
	BMessenger msgr;

	BAlert *alert;
	bigtime_t delay;
	int32 mode;

	switch(message->what)
	{
		case MSG_TOGGLE_ACTIVE:
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
			thread_id tid = spawn_thread(removeFromDeskbar, "RemoveFromDeskbar", B_NORMAL_PRIORITY, (void*)this);
			if (tid != 0) resume_thread(tid);

			break;
		}
		case B_ABOUT_REQUESTED:
			alert = new BAlert("about box",
				B_TRANSLATE("AutoRaise, (c) 2002, mmu_man\nEnjoy :-)"),
				B_TRANSLATE("OK"), NULL, NULL,
				B_WIDTH_AS_USUAL, B_OFFSET_SPACING, B_INFO_ALERT);
			alert->SetShortcut(0, B_ENTER);
			alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
			alert->Go(NULL); // use asynchronous version
			break;
		case OPEN_SETTINGS:

			break;

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
			release_sem(fPollerSem);
			watching = true;
		}
	}
	else
	{
		if (watching) {
			acquire_sem(fPollerSem);
			set_mouse_mode(fNormalMM);
			watching = false;
		}
	}
	Invalidate();
}

