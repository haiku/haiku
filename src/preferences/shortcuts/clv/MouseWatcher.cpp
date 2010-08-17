#include "MouseWatcher.h"

#include <Messenger.h>
#include <InterfaceKit.h>

int32 MouseWatcher(void* data);


thread_id StartMouseWatcher(BView* TargetView)
{
	thread_id MouseWatcherThread = spawn_thread(MouseWatcher,"MouseWatcher",B_NORMAL_PRIORITY,
		new BMessenger(TargetView));
	if(MouseWatcherThread != B_NO_MORE_THREADS && MouseWatcherThread != B_NO_MEMORY)
		resume_thread(MouseWatcherThread);
	return MouseWatcherThread;
}


int32 MouseWatcher(void* data)
{
	BMessenger* TheMessenger = (BMessenger*)data;
	BPoint PreviousPos;
	uint32 PreviousButtons = 0xFFFFFFFF;
	bool FirstCheck = true;
	BMessage MessageToSend;
	MessageToSend.AddPoint("where",BPoint(0,0));
	MessageToSend.AddInt32("buttons",0);
	MessageToSend.AddInt32("modifiers",0);
	while(true)
	{
		if (!TheMessenger->LockTarget())
		{
			delete TheMessenger;
			return 0;			// window is dead so exit
		}
		BLooper *TheLooper;
		BView* TheView = (BView*)TheMessenger->Target(&TheLooper);
		BPoint Where;
		uint32 Buttons;
		TheView->GetMouse(&Where,&Buttons,false);
		if(FirstCheck)
		{
			PreviousPos = Where;
			PreviousButtons = Buttons;
			FirstCheck = false;
		}
		bool Send = false;
		if(Buttons != PreviousButtons || Buttons == 0 || Where != PreviousPos)
		{
			if(Buttons == 0)
				MessageToSend.what = MW_MOUSE_UP;
			else if(Buttons != PreviousButtons)
				MessageToSend.what = MW_MOUSE_DOWN;
			else
				MessageToSend.what = MW_MOUSE_MOVED;
			MessageToSend.ReplacePoint("where",Where);
			MessageToSend.ReplaceInt32("buttons",Buttons);
			MessageToSend.ReplaceInt32("modifiers",modifiers());
			Send = true;
		}
		TheLooper->Unlock();
		if(Send)
			TheMessenger->SendMessage(&MessageToSend);
		if(Buttons == 0)
		{
			//Button was released
			delete TheMessenger;
			return 0;
		}
		snooze(50000);
	}
}
