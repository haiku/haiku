#ifndef AUTORAISEAPP_H
#define AUTORAISEAPP_H

#include <Application.h>
#include <Archivable.h>

#include "common.h"
#include "settings.h"


class AutoRaiseApp: public BApplication{

	public:
		AutoRaiseApp();
		virtual ~AutoRaiseApp();
		virtual bool QuitRequested() { return true; }
		virtual void ArgvReceived(int32 argc, char ** args);		
		virtual void ReadyToRun();
		
		void PutInTray(bool);

	private:
		bool fPersist;
		bool fDone;
};

#endif
