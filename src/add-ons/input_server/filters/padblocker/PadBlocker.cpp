/*
 Touchpad sensitivity filter
 Most windows/mac/etc laptops have a sensitivity filter/mechanism
 which protects the user from accidently rubbing the
 touchpad while typing and thus deleting text or moving
 the cursor to a new insertion point. I've been grousing
 about this to myself for months, and finally decided to
 write something for BeOS which other people might benefit
 from.

 Just an input_server add_on. I wrote it in < hour. Works on
 my inspiron 3500, but I imagine it would work fine on any BeOS
 machine. Not that desktops would benefit. Compiled under 4.5.2

 Do what you want with it. Enjoy.

 --Shamyl Zakariya
 --March 22 2000
 --zakariya@earthlink.net

 Note:
 reads Touchpad_settings file in home/config/settings .
 padblocker_threshold value represents the delay between the last
 B_KEY_UP message and when the filter will allow
 a B_MOUSE_DOWN message. Eg, if less than _threshold has
 transpired between a B_KEY_DOWN and a B_MOUSE_DOWN, the mouse
 down event will be skipped, cast into oblivion.

 Kudos to the people at Be for making this kind of thing
 so damnably easy to do. Bravo!

 And a hearty thanks to Magnus Landahl for his superb
 debug console, without which I wouldn't have ever known if
 this damn thing worked at all.

*/


#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <input/touchpad_settings.h>
#include <Debug.h>
#include <List.h>
#include <Message.h>
#include <OS.h>
#include <StorageKit.h>

#include <add-ons/input_server/InputServerFilter.h>

#if DEBUG
#	define LOG(text...) PRINT((text))
#else
#	define LOG(text...)
#endif

//#define Z_DEBUG 1


#if Z_DEBUG
#include <BeDC.h>
#endif

extern "C" _EXPORT BInputServerFilter* instantiate_input_filter();

class PadBlocker : public BInputServerFilter
{
	public:
		PadBlocker();
		virtual ~PadBlocker();
		virtual	filter_result Filter(BMessage *message, BList *outList);

	private:
		bigtime_t 			_lastKeyUp; //timestamp of last B_KEY_DOWN
		bigtime_t 			_threshold;
		status_t			GetSettingsPath(BPath& path);
		BPoint				fWindowPosition;
};


status_t
PadBlocker::GetSettingsPath(BPath &path)
{
	status_t status = find_directory(B_USER_SETTINGS_DIRECTORY, &path);
	if (status < B_OK)
		return status;

	return path.Append(TOUCHPAD_SETTINGS_FILE);
}


PadBlocker::PadBlocker()
{
	_lastKeyUp = 0;
	_threshold = 300; //~one third of a second?

	touchpad_settings settings;

	#if Z_DEBUG
	BeDC dc("PadBlocker");
	#endif
	BPath path;
	status_t status = GetSettingsPath(path);
	if (status == B_OK)
	{

		BFile settingsFile(path.Path(), B_READ_ONLY);
		status = settingsFile.InitCheck();
		if (status == B_OK)
		{

			if (settingsFile.Read(&settings, sizeof(touchpad_settings))
				!= sizeof(touchpad_settings)) {
				LOG("failed to load settings\n");
				status = B_ERROR;
			}
			if (settingsFile.Read(&fWindowPosition, sizeof(BPoint))
				!= sizeof(BPoint)) {
				LOG("failed to load settings\n");
				status = B_ERROR;
			}
		}
	}

	//load settings file

	if (status == B_OK)
	{
		#if Z_DEBUG
		dc.SendMessage("Settings file Exists");
		#endif
		_threshold = settings.padblocker_threshold;
	}



	_threshold *= 1000; //I'd rather keep the number in the file in thousandths

	#ifdef Z_DEBUG
	dc.SendInt(_threshold, "Touchpad threshold.");
	#endif
}


PadBlocker::~PadBlocker()
{
}


filter_result PadBlocker::Filter(BMessage *message, BList *outList)
{
	filter_result res = B_DISPATCH_MESSAGE;

	switch (message->what)
	{
		case B_KEY_UP: case B_KEY_DOWN:
		{
			_lastKeyUp = system_time();	//update timestamp
			break;
		}

		case B_MOUSE_DOWN:
		{
			// do nothing if disabled
			if (_threshold == 0)
				break;

			bigtime_t now = system_time();
			// if less than the threshold has passed, tell input server
			// to ignore this message
			if ((now - _lastKeyUp) < _threshold) res = B_SKIP_MESSAGE;
			break;
		}

		default:
		{
			//we don't want to mess with anybody else
			break;
		}
	}

	#if Z_DEBUG
	if (res == B_SKIP_MESSAGE)
	{
		BeDC dc("PadBlocker");
		dc.SendMessage("Skipping mouse down event");
	}
	#endif

	return (res);
}


//*************************************************************************
//Exported instantiator

BInputServerFilter* instantiate_input_filter()
{
	return new (std::nothrow) PadBlocker();
}
