#ifndef ICON_H
#define ICON_H

#include <Application.h>
#include <Archivable.h>

#include "common.h"
#include "settings.h"


class AutoRaiseApp: public BApplication{
	protected:
		bool _directToDeskbar;

	public:
		AutoRaiseApp();
		virtual ~AutoRaiseApp();
		virtual bool QuitRequested() { return true; }
		virtual void ArgvReceived(int32 argc, char ** args);		
		virtual void ReadyToRun();
		
		void PutInTray(bool);

	private:
		BString usageNote;
};

#endif
