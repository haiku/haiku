#include "FileTypeConstants.h"
#include <TrackerAddOn.h>
#include <Roster.h>

extern "C" void
process_refs(entry_ref dir_ref, BMessage * message, void *)
{
	be_roster->Launch(APP_SIGNATURE,message);
}
