//
// CannaIM - Canna-based Input Method Add-on for BeOS R4
//

#include <List.h>
#include <Looper.h>
#include <FindDirectory.h>
#include <Path.h>
#include <File.h>

#include "CannaCommon.h"
#include "CannaMethod.h"
#include "CannaLooper.h"

#include "Bitmaps.h"

#ifdef DEBUG
#include <Debug.h>
#endif

Preferences gSettings;

BInputServerMethod*
instantiate_input_method()
{
	return new CannaMethod();
}


//	#pragma mark -


CannaMethod::CannaMethod()
	: BInputServerMethod("Canna", (const uchar*)kCannaIcon)
{
#ifdef DEBUG
	SERIAL_PRINT(( "CannaIM:Constructor called.\n" ));
#endif
	ReadSettings();
}

CannaMethod::~CannaMethod()
{
	BLooper *looper = NULL;
	cannaLooper.Target( &looper );
	if ( looper != NULL )
	{
#ifdef DEBUG
	SERIAL_PRINT(( "CannaIM:Locking CannaLooper...\n" ));
#endif
		if ( looper->Lock() )
#ifdef DEBUG
	SERIAL_PRINT(( "CannaIM:CannaLooper locked. Calling Quit().\n" ));
#endif
			looper->Quit();
	}
	WriteSettings();
}

status_t
CannaMethod::MethodActivated( bool active )
{
	BMessage msg( CANNA_METHOD_ACTIVATED );

	if ( active )
		msg.AddBool( "active", true );

	cannaLooper.SendMessage( &msg );

	return B_OK;
}

filter_result
CannaMethod::Filter( BMessage *msg, BList *outList )
{
	if ( msg->what != B_KEY_DOWN )
		return B_DISPATCH_MESSAGE;

	cannaLooper.SendMessage( msg );
	return B_SKIP_MESSAGE;
}

status_t
CannaMethod::InitCheck()
{
#ifdef DEBUG
	SERIAL_PRINT(( "CannaIM:InitCheck() called.\n" ));
#endif
	CannaLooper *looper;
	status_t err;
	looper = new CannaLooper( this );
	looper->Lock();
	err = looper->Init();
	looper->Unlock();
	cannaLooper = BMessenger( NULL, looper );
#ifdef DEBUG
	if ( err != B_NO_ERROR )
	SERIAL_PRINT(( "CannaLooper::InitCheck() failed.\n" ));
	else
	SERIAL_PRINT(( "CannaLooper::InitCheck() success.\n" ));
#endif

	return err;
}

void
CannaMethod::ReadSettings()
{
	BMessage pref;
	BFile preffile( CANNAIM_SETTINGS_FILE, B_READ_ONLY );
	if ( preffile.InitCheck() == B_NO_ERROR && pref.Unflatten( &preffile ) == B_OK )
	{
		pref.FindBool( "arrowkey", &gSettings.convert_arrowkey );
		pref.FindPoint( "palette", &gSettings.palette_loc );
		pref.FindPoint( "standalone", &gSettings.standalone_loc );
#ifdef DEBUG
SERIAL_PRINT(( "CannaMethod: ReadSettings() success. arrowkey=%d, palette_loc=%d,%d standalone_loc=%d, %d\n", gSettings.convert_arrowkey, gSettings.palette_loc.x, gSettings.palette_loc.y, gSettings.standalone_loc.x, gSettings.standalone_loc.y ));
#endif
		return;
	}

	//set default values
#ifdef DEBUG
SERIAL_PRINT(( "CannaMethod: ReadSettings() failed.\n" ));
#endif
	gSettings.convert_arrowkey = true;
	gSettings.palette_loc.Set( 800, 720 );
	gSettings.standalone_loc.Set( 256, 576 );
}

void CannaMethod::WriteSettings()
{
	BMessage pref;
	BFile preffile( CANNAIM_SETTINGS_FILE,
			B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE );

	if ( preffile.InitCheck() == B_NO_ERROR )
	{
		pref.AddBool( "arrowkey", gSettings.convert_arrowkey );
		pref.AddPoint( "palette", gSettings.palette_loc );
		pref.AddPoint( "standalone", gSettings.standalone_loc );
		pref.Flatten( &preffile );
#ifdef DEBUG
SERIAL_PRINT(( "CannaMethod: WriteSettings() success. arrowkey=%d, palette_loc=%d,%d standalone_loc=%d, %d\n", gSettings.convert_arrowkey, gSettings.palette_loc.x, gSettings.palette_loc.y, gSettings.standalone_loc.x, gSettings.standalone_loc.y ));
#endif
	}
}
