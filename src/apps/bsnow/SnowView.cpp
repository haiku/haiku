#include "SnowView.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <Alert.h>
#include <Catalog.h>
#include <Debug.h>
#include <Message.h>
#include <MessageRunner.h>
#include <MessageFilter.h>
#include <OS.h>
#include <Region.h>
#include <Screen.h>

#include "Flakes.h"


#define FORWARD_TO_PARENT
#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "BSnow"


SnowView::SnowView()
	:
	BView(BRect(SNOW_VIEW_RECT), "BSnow", B_FOLLOW_NONE,
	B_WILL_DRAW | B_PULSE_NEEDED)
{
	fAttached = false;
	fMsgRunner = NULL;
	fCachedParent = NULL;
	fFallenBmp = NULL;
	fFallenView = NULL;
	fFallenReg = NULL;
	fInvalidator = -1;
	fShowClickMe = false;
	for (int i = 0; i < WORKSPACES_COUNT; i++)
		fFlakes[i] = NULL;
	for (int i = 0; i < NUM_PATTERNS; i++)
		fFlakeBitmaps[i] = NULL;
	BRect r(Frame());
	r.left = r.right - 7;
	r.top = r.bottom - 7;
	fDragger = new BDragger(r, this);
	AddChild(fDragger);
	SetHighColor(255,255,255);
}

#ifdef DEBUG
filter_result msgfilter(BMessage *message, BHandler **target, BMessageFilter *filter)
{
	switch (message->what) {
	case B_MOUSE_DOWN:
	case B_MOUSE_UP:
	case B_MOUSE_MOVED:
	case '_EVP':
	case '_UPD':
	case '_PUL':
	case 'NTCH':
	case 'NMDN':
		break;
	default:
		printf("For: %p: %s\n", *target, (*target)->Name());
		message->PrintToStream();
	}
	return B_DISPATCH_MESSAGE;
}
#endif

SnowView::SnowView(BMessage *archive)
 : BView(archive)
{
	system_info si;
	PRINT(("SnowView()\n"));
#ifdef DEBUG
	archive->PrintToStream();
#endif
	fDragger = NULL;
	fAttached = false;
	fMsgRunner = NULL;
	fFallenBmp = NULL;
	fFallenView = NULL;
	fFallenReg = NULL;
	fCachedParent = NULL;
	fShowClickMe = false;
	SetFlags(Flags() & ~B_PULSE_NEEDED); /* it's only used when in the app */
	get_system_info(&si);
	fNumFlakes = ((int32)(si.cpu_clock_speed/1000000)) * si.cpu_count / 3; //;
	printf("BSnow: using %ld flakes\n", fNumFlakes);
	for (int i = 0; i < WORKSPACES_COUNT; i++) {
		fFlakes[i] = new flake[fNumFlakes];
		memset(fFlakes[i], 0, fNumFlakes * sizeof(flake));
	}
	for (int i = 0; i < NUM_PATTERNS; i++) {
		fFlakeBitmaps[i] = new BBitmap(BRect(0,0,7,7), B_CMAP8);
		fFlakeBitmaps[i]->SetBits(gFlakeBits[i], 8*8, 0, B_CMAP8);
	}
	fCurrentWorkspace = 0;
	SetHighColor(255,255,255);
	SetDrawingMode(B_OP_OVER);
}

SnowView::~SnowView()
{
	for (int i = 0; i < WORKSPACES_COUNT; i++)
		if (fFlakes[i])
			delete [] fFlakes[i];
	if (fFallenBmp)
		delete fFallenBmp; /* the view goes away with it */
}

BArchivable *SnowView::Instantiate(BMessage *data)
{
	return new SnowView(data);
}

status_t SnowView::Archive(BMessage *data, bool deep) const
{
	status_t err;
	err = BView::Archive(data, deep);
	if (err < B_OK)
		return err;
	data->AddString("add_on", APP_SIG);
	return B_OK;
}

void SnowView::AttachedToWindow()
{
	BView *p;
	rgb_color col;
	fAttached = true;
/*	if (!fMsgRunner)
		fMsgRunner = new BMessageRunner(BMessenger(this), 
					new BMessage(MSG_PULSE_ME), 
					INTERVAL);
*/
	p = Parent();
	if (p)
		col = B_TRANSPARENT_32_BIT;//Parent()->ViewColor();
	else
		col = ui_color(B_PANEL_BACKGROUND_COLOR);
	SetViewColor(col);
//	BScreen bs;
//	fCachedWsWidth = bs.Frame().IntegerWidth();
//	fCachedWsHeight = bs.Frame().IntegerHeight();
	fDragger = dynamic_cast<BDragger *>(FindView("_dragger_"));
	if (fDragger && p) {
		fCachedParent = p;
		fCachedWsWidth = p->Frame().IntegerWidth();
		fCachedWsHeight = p->Frame().IntegerHeight();
		fDragger->SetViewColor(col);
		if (fDragger->InShelf()) {
			p->SetFlags(p->Flags() | B_DRAW_ON_CHILDREN);
#ifdef B_BEOS_VERSION_DANO
			p->SetDoubleBuffering(p->DoubleBuffering() | B_UPDATE_EXPOSED);
#endif
			ResizeTo(p->Bounds().Width(), p->Bounds().Height());
			MoveTo(0,0);
			fDragger->MoveTo(p->Bounds().Width()-7, p->Bounds().Height()-7);
		}
		BRect fallenRect(p->Bounds());
		fallenRect.top = fallenRect.bottom - FALLEN_HEIGHT;
		fFallenBmp = new BBitmap(fallenRect, B_BITMAP_ACCEPTS_VIEWS, B_CMAP8);
		memset(fFallenBmp->Bits(), B_TRANSPARENT_MAGIC_CMAP8, (size_t)(fallenRect.Height()*fFallenBmp->BytesPerRow()));
		fFallenView = new BView(fallenRect, "offscreen fallen snow", B_FOLLOW_NONE, 0);
		fFallenBmp->AddChild(fFallenView);
		fFallenReg = new BRegion;
		fInvalidator = spawn_thread(SnowMakerThread, INVALIDATOR_THREAD_NAME, B_LOW_PRIORITY, (void *)this);
		resume_thread(fInvalidator);
		printf("BSnow: OK: ws = %ld x %ld\n", fCachedWsWidth, fCachedWsHeight);
#ifdef DEBUG
		Window()->AddCommonFilter(new BMessageFilter(B_ANY_DELIVERY, B_ANY_SOURCE, msgfilter));
#endif
	}
}

void SnowView::DetachedFromWindow()
{
	fAttached = false;
/*
	if (Parent()) {
		Parent()->Invalidate(Parent()->Bounds());
	}
*/
	if (fMsgRunner)
		delete fMsgRunner;
	fMsgRunner = NULL;
	status_t err;
	fCachedParent = NULL;
	if (fInvalidator > B_OK)
		wait_for_thread(fInvalidator, &err);
	fInvalidator = -1;
	if (fFallenReg)
		delete fFallenReg;
}

void SnowView::MessageReceived(BMessage *msg)
{
	BAlert *info;
	//msg->PrintToStream();
	switch (msg->what) {
	case MSG_PULSE_ME:
		if (Parent()) {
			Calc();
			InvalFlakes();
		}
		break;
	case B_ABOUT_REQUESTED:
		info = new BAlert("BSnow info", 
			"BSnow, just in case you don't have real one...\n"
			"" B_UTF8_COPYRIGHT " 2003, François Revol.", 
			"Where is Santa ??");
		info->SetFeel(B_NORMAL_WINDOW_FEEL);
		info->SetLook(B_FLOATING_WINDOW_LOOK);
		info->SetFlags(info->Flags()|B_NOT_ZOOMABLE|B_CLOSE_ON_ESCAPE);
		info->Go(NULL);
		break;
	default:
//#ifdef FORWARD_TO_PARENT
/*
		if (fAttached && Parent())
			Parent()->MessageReceived(msg);
		else
*/
//#endif
			BView::MessageReceived(msg);
	}
}

void SnowView::Draw(BRect ur)
{
	int i;
	if (!fCachedParent) {
		if (!fShowClickMe) { /* show "drag me" */
			SetLowColor(ViewColor());
			SetHighColor(0,0,0);
			SetFontSize(12);
			DrawString(B_TRANSLATE("Drag me on your desktop"B_UTF8_ELLIPSIS),
				BPoint(15,25));
			BPoint arrowHead(Bounds().RightBottom() + BPoint(-10,-10));
			StrokeLine(arrowHead, arrowHead - BPoint(7,0));
			StrokeLine(arrowHead, arrowHead - BPoint(0,7));
			StrokeLine(arrowHead, arrowHead - BPoint(12,12));
			return;
		} else {
			SetLowColor(ViewColor());
			SetHighColor(0,0,0);
			SetFontSize(12);
			DrawString(B_TRANSLATE("Click me to remove BSnow"B_UTF8_ELLIPSIS),
				BPoint(15,25));
			return;
		}
	}
	//printf("Draw()\n");
	uint32 cw = fCurrentWorkspace;
	if (fFlakes[cw] == NULL)
		return;
	/* draw the snow already fallen */
//	BRect fallenRect(Bounds());
//	fallenRect.top = fallenRect.bottom - FALLEN_HEIGHT;
//	if (ur.Intersects(fallenRect)) {
		//if (fFallenBmp->Lock()) {
		//	DrawBitmap(fFallenBmp, fallenRect);
		//	fFallenBmp->Unlock();
		//}
		int32 cnt = fFallenReg->CountRects();
//		drawing_mode oldmode = DrawingMode();
//		SetDrawingMode(B_OP_ADD);

		for (i=0; i<cnt; i++) {
			BRect r = fFallenReg->RectAt(i);
//			SetHighColor(245, 245, 245, 200);
//			FillRect(r);
//			SetHighColor(255, 255, 255, 255);
//			r.InsetBy(1,1);
			FillRect(r);
		}
//		SetDrawingMode(oldmode);
//	}
	/* draw our flakes */
	for (i=0; i<fNumFlakes; i++) {
		int pat;
		if (!ur.Contains(BRect(fFlakes[cw][i].pos-BPoint(4,4), fFlakes[cw][i].pos+BPoint(4,4))))
			continue;
		if (fFlakes[cw][i].weight == 0)
			continue;
		pat = (fFlakes[cw][i].weight>3)?1:0;
		//FillRect(BRect(fFlakes[cw][i].pos-BPoint(PAT_HOTSPOT),fFlakes[cw][i].pos-BPoint(PAT_HOTSPOT)+BPoint(7,7)), gFlakePatterns[pat]);
/*
		StrokeLine(fFlakes[cw][i].pos+BPoint(-1,-1), 
				fFlakes[cw][i].pos+BPoint(1,1));
		StrokeLine(fFlakes[cw][i].pos+BPoint(-1,1), 
				fFlakes[cw][i].pos+BPoint(1,-1));
*/
		DrawBitmap(fFlakeBitmaps[pat], fFlakes[cw][i].pos-BPoint(PAT_HOTSPOT));
	}
}

void SnowView::Pulse()
{
	if (fShowClickMe)
		return; /* done */
	if (fCachedParent)
		return; /* we are in Tracker! */
	BMessenger msgr("application/x-vnd.Be-TRAK");
	BMessage msg(B_GET_PROPERTY), reply;
	msg.AddSpecifier("Frame");
	msg.AddSpecifier("View", "BSnow");
	msg.AddSpecifier("Window", 1); /* 0 is Twitcher */
	if (msgr.SendMessage(&msg, &reply) == B_OK && reply.what == B_REPLY) {
		//reply.PrintToStream();
		Invalidate(Bounds());
		fShowClickMe = true;
	}
}

void SnowView::Calc()
{
	int i;
	uint32 cw = fCurrentWorkspace;

	/* check if the parent changed size */
	BRect pFrame = fCachedParent->Frame();
	if (fCachedWsWidth != pFrame.Width() || fCachedWsHeight != pFrame.Height()) {
		fCachedWsWidth = pFrame.IntegerWidth();
		fCachedWsHeight = pFrame.IntegerHeight();
		printf("BSnow: Parent resized to %ld %ld\n", fCachedWsWidth, fCachedWsHeight);
		fFallenReg->MakeEmpty(); /* remove all the fallen snow */
		ResizeTo(pFrame.IntegerWidth(), pFrame.IntegerHeight());
		fDragger->MoveTo(pFrame.IntegerWidth()-7, pFrame.IntegerHeight()-7);
	}

	/* make new flakes */
	for (i=0; i<fNumFlakes; i++) {
		if (fFlakes[cw][i].weight == 0) {
			fFlakes[cw][i].weight = ((float)(rand() % WEIGHT_SPAN)) / WEIGHT_GRAN;
			fFlakes[cw][i].weight = MAX(fFlakes[cw][i].weight, 0.5);
			fFlakes[cw][i].pos.y = rand() % 5 - 2;
			fFlakes[cw][i].pos.x = (rand()%(fCachedWsWidth+2*fCachedWsHeight))-fCachedWsHeight;
			if (fFlakes[cw][i].pos.x < -10) {
				fFlakes[cw][i].pos.y = -fFlakes[cw][i].pos.x;
				if (fWind > 0)
					fFlakes[cw][i].pos.x = 0;
				else
					fFlakes[cw][i].pos.x = fCachedWsWidth;
			}
			if (fFlakes[cw][i].pos.x > fCachedWsWidth+10) {
				fFlakes[cw][i].pos.y = fFlakes[cw][i].pos.x - fCachedWsWidth;
				if (fWind > 0)
					fFlakes[cw][i].pos.x = 0;
				else
					fFlakes[cw][i].pos.x = fCachedWsWidth;
			}
		}
	}
	
	/* like a candle in the wind... */
	if (fWindDuration < system_time()) {
		fWindDuration = system_time() + ((((bigtime_t)rand())*1000) % WIND_MAX_DURATION);
		fWind = (rand() % WIND_SPAN) - WIND_SPAN/2;
		printf("BSnow: wind change: %f\n", fWind);
	}
	

//	if (fFallenView->LockLooperWithTimeout(5000)) {
//	if (fFallenBmp) {
//		uint8 *fallenBits = (uint8 *)fFallenBmp->Bits();
		
		BRegion desktopReg;
		GetClippingRegion(&desktopReg);

		/* let's add some gravity and wind */
		for (i=0; i<fNumFlakes; i++) {
			float yinc;
			if (fFlakes[cw][i].weight == 0)
				continue;
			fFlakes[cw][i].opos = fFlakes[cw][i].pos;
			
			yinc = fFlakes[cw][i].weight - (rand() % 3);
			yinc = MAX(yinc, 0.5);
			fFlakes[cw][i].pos.y += yinc;

//			if (fFlakes[cw][i].pos.y > (fCachedWsHeight-FALLEN_HEIGHT)) {

			bool fallen = false;
			bool keepfalling = false;
			
			/* fallen on the flour */
			if (fFlakes[cw][i].pos.y > fCachedWsHeight-2)
				fallen = true;
			/* fallon on another fallen flake */
			else if (fFallenReg->Intersects(BRect(fFlakes[cw][i].pos - BPoint(0,1), 
											fFlakes[cw][i].pos + BPoint(0,1)))) {
				/* don't accumulate too much */
				if ((fFlakes[cw][i].pos.y > fCachedWsHeight-30) || 
									!desktopReg.Intersects(
									BRect(fFlakes[cw][i].pos + BPoint(0,6), 
									fFlakes[cw][i].pos + BPoint(0,10))))
					fallen = true;
			/* fallen on a window */
			} else if (!desktopReg.Intersects(
							BRect(fFlakes[cw][i].pos + BPoint(-1,-1-2), 
							fFlakes[cw][i].pos + BPoint(1,1-1))) && 
					desktopReg.Intersects(
							BRect(fFlakes[cw][i].pos + BPoint(-1,-1-3), 
							fFlakes[cw][i].pos + BPoint(1,1-3)))) {
				//printf("fallen3 @ %f %f\n", fFlakes[cw][i].pos.x, fFlakes[cw][i].pos.y);
				fFlakes[cw][i].pos = fFlakes[cw][i].opos;
				fallen = true;
				keepfalling = true; /* but keep one falling */
			}
			
/*			else if (fallenBits[ (long)(fFlakes[cw][i].pos.y
					 * fFallenBmp->BytesPerRow()
					 + fFlakes[cw][i].pos.y
					 - (fCachedWsHeight-FALLEN_HEIGHT)) ] != B_TRANSPARENT_MAGIC_CMAP8) {
				fallen = true;
			}*/

//			if (fallen) {
//				int pat = (fFlakes[cw][i].weight>3)?1:0;
//				if (fFlakes[cw][i].pos.y > fCachedWsHeight-1)
//					fFlakes[cw][i].pos.y = fCachedWsHeight-(rand()%4);
				//fFallenView->DrawBitmap(fFlakeBitmaps[pat], fFlakes[cw][i].pos-BPoint(PAT_HOTSPOT));
//				fallenBits[ (long)(fFlakes[cw][i].pos.y * fFallenBmp->BytesPerRow()
//					+ fFlakes[cw][i].pos.y-(fCachedWsHeight-FALLEN_HEIGHT)) ] = 0x56;
//				printf("fallen @ %f, %f\n", fFlakes[cw][i].pos.x, fFlakes[cw][i].pos.y);
//			}
			if (fallen) {
				if (!keepfalling)
					fFlakes[cw][i].weight = 0;
				fFallenReg->Include(BRect(fFlakes[cw][i].pos - BPoint(2,0), 
										fFlakes[cw][i].pos + BPoint(2,2)));
				if (keepfalling) {
					fFlakes[cw][i].pos += BPoint(0,10);
					/* except if under the desktop */
					if (fFlakes[cw][i].pos.y > fCachedWsHeight-1)
						fFlakes[cw][i].weight = 0;
				}
			}
			
			/* cleanup, when a window hides the snow */
			fFallenReg->IntersectWith(&desktopReg);
			
			/* cleanup, when a window is moved */
			/* seems to lockup Tracker */
/*
			int32 cnt = fFallenReg->CountRects();
			for (i=0; i<cnt; i++) {
				BRect r = fFallenReg->RectAt(i);
				if (desktopReg.Intersects(r.OffsetByCopy(0,15))) {
					fFallenReg->Exclude(r);
					cnt--;
				}
			}
*/			
			/* add the effect of the wind */
			fFlakes[cw][i].pos.x += fWind + (rand() % 6 - 3);
			if ((fFlakes[cw][i].pos.x > fCachedWsWidth+50)||(fFlakes[cw][i].pos.x < -50))
				fFlakes[cw][i].weight = 0;
		}
//		fFallenView->UnlockLooper();
//	}
#if 0
	for (i=0; i<10; i++)
		printf("f[%d] = {%f, %f}, {%f, %f}, %d\n", i,
			fFlakes[cw][i].opos.x, fFlakes[cw][i].opos.y,
			fFlakes[cw][i].pos.x, fFlakes[cw][i].pos.y,
			fFlakes[cw][i].weight);
#endif
}

void SnowView::InvalFlakes()
{
	int i;
	BView *p = Parent();
	if (!p)
		return;
	//printf("InvalFlakes()\n");
	uint32 cw = fCurrentWorkspace;
	
	for (i=0; i<fNumFlakes; i++) {
		if (fFlakes[cw][i].weight)
			Invalidate(BRect(fFlakes[cw][i].opos-BPoint(PAT_HOTSPOT), fFlakes[cw][i].opos-BPoint(PAT_HOTSPOT)+BPoint(7,7)));
	}
}

void SnowView::MouseDown(BPoint where)
{
#ifdef FORWARD_TO_PARENT
	if (fAttached && Parent())
		Parent()->MouseDown(where);
#endif
}

void SnowView::MouseUp(BPoint where)
{
#ifdef FORWARD_TO_PARENT
	if (fAttached && Parent())
		Parent()->MouseUp(where);
#endif
	if (fCachedParent)
		return; /* we are *inside* the Tracker,
				 * don't even try talking to ourselve
				 * with the window locked
				 */
	BMessenger msgr("application/x-vnd.Be-TRAK");
	BMessage msg(B_DELETE_PROPERTY), reply;
	msg.AddSpecifier("Replicant", "BSnow");
	msg.AddSpecifier("Shelf");
	msg.AddSpecifier("View", "PoseView");
	msg.AddSpecifier("Window", 1); /* 0 is Tracker Status */
	if ((msgr.SendMessage(&msg, &reply) == B_OK) && 
				(reply.what == B_NO_REPLY || reply.what == B_REPLY)) {
		//reply.PrintToStream();
		fShowClickMe = false;
		Invalidate(Bounds());
	}
	/*
	BMessage: what = JAHA (0x4a414841, or 1245792321)
    entry          index, type='LONG', c=1, size= 4, data[0]: 0x2 (2, '')
    entry           when, type='LLNG', c=1, size= 8, data[0]: 0xf6a1b09ac (66204666284, '')
    entry         source, type='PNTR', c=1, size= 4,
    entry      be:sender, type='MSNG', c=1, size=24,
    */
}

void SnowView::MouseMoved(BPoint where, uint32 code, const BMessage *a_message)
{
#ifdef FORWARD_TO_PARENT
	if (fAttached && Parent())
		Parent()->MouseMoved(where, code, a_message);
#endif
}

void SnowView::KeyDown(const char *bytes, int32 numBytes)
{
#ifdef FORWARD_TO_PARENT
	if (fAttached && Parent())
		Parent()->KeyDown(bytes, numBytes);
#endif
}

void SnowView::KeyUp(const char *bytes, int32 numBytes)
{
#ifdef FORWARD_TO_PARENT
	if (fAttached && Parent())
		Parent()->KeyUp(bytes, numBytes);
#endif
}

#define PORTION_GRAN 20

int32 SnowView::SnowMakerThread(void *p_this)
{
	SnowView *_this = (SnowView *)p_this;
	BView *p = _this->Parent();
	BRect portion(0,0,(_this->fCachedWsWidth/PORTION_GRAN)-1, (_this->fCachedWsHeight/PORTION_GRAN)-1);
	int nf = _this->fNumFlakes;
	BRegion reg(BRect(-1,-1,-1,-1));
	while (p && _this->fAttached) {
		snooze(INTERVAL/(10*(nf?nf:1)));
		int32 cw = _this->fCurrentWorkspace;
		bool drawThisOne = false;
//printf("processing flake %d...\n", current);
		//for (; (current%(fNumFlakes/4); current++)
		if (reg.Intersects(portion)) {
			for (int i = 0; !drawThisOne && i < nf; i++) {
				/* if we find at least one flake in this rect, draw it */
				if ((_this->fFlakes[cw][i].weight) && (
					portion.Intersects(BRect(_this->fFlakes[cw][i].opos - BPoint(4,4), _this->fFlakes[cw][i].opos + BPoint(4,4))) || 
					portion.Intersects(BRect(_this->fFlakes[cw][i].pos - BPoint(4,4), _this->fFlakes[cw][i].pos + BPoint(4,4))))) {
						drawThisOne = true;
				}
			}
		}
		//if (!drawThisOne)
			//printf("!Invalidate(%f, %f, %f, %f)\n", portion.left, portion.top, portion.right, portion.bottom);
		/* avoid deadlock on exit */
		if (drawThisOne && (_this->LockLooperWithTimeout(2000) == B_OK)) {
//			printf("Invalidate(%f, %f, %f, %f)\n", portion.left, portion.top, portion.right, portion.bottom);
			p->Invalidate(portion);
			_this->UnlockLooper();
		}
		portion.OffsetBy(_this->fCachedWsWidth/PORTION_GRAN, 0);
		if (portion.left >= _this->fCachedWsWidth) { /* right wrap */
			//printf("rigth wrap to %ld\n", _this->fCachedWsWidth);
			portion.OffsetTo(0, portion.top+(_this->fCachedWsHeight/PORTION_GRAN));
		}
		if (portion.top >= _this->fCachedWsHeight) {
			portion.OffsetTo(0,0);
			/* avoid deadlock on exit */
			if (_this->LockLooperWithTimeout(5000) == B_OK) {
//printf("calculating flakes...\n");
				_this->Calc();
//printf("done calculating flakes.\n");
				_this->GetClippingRegion(&reg);
				//printf("Region:\n");
				//reg.PrintToStream();
				_this->UnlockLooper();
			}
		}	
	}
#if 0
	BView *p = _this->Parent();
	while (p && _this->fAttached) {
		snooze(INTERVAL/_this->fNumFlakes);
//printf("processing flake %d...\n", current);
		//for (; (current%(fNumFlakes/4); current++)
		/* avoid deadlock on exit */
		if (_this->LockLooperWithTimeout(2000) == B_OK) {
			if (_this->fFlakes[_this->fCurrentWorkspace][current].weight) {
				p->Invalidate(BRect(_this->fFlakes[_this->fCurrentWorkspace][current].opos - BPoint(4,4),
									_this->fFlakes[_this->fCurrentWorkspace][current].opos + BPoint(4,4)));
				p->Invalidate(BRect(_this->fFlakes[_this->fCurrentWorkspace][current].pos - BPoint(4,4),
									_this->fFlakes[_this->fCurrentWorkspace][current].pos + BPoint(4,4)));
			}
			_this->UnlockLooper();
			current++;
			current %= _this->fNumFlakes;
			if (!current) {
				/* avoid deadlock on exit */
				if (_this->LockLooperWithTimeout(2000) == B_OK) {
printf("calculating flakes...\n");
					_this->Calc();
printf("done calculating flakes.\n");
					_this->UnlockLooper();
				}
			}	
		}
	}
#endif
	return B_OK;
}
