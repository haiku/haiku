#include <support/Autolock.h>
#include <media/MediaDefs.h>

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <media/MediaAddOn.h>

#include <Directory.h>
#include <Entry.h>
#include <Path.h>
#include <String.h>

#include "LegacyMediaAddOn.h"

#include "LegacyAudioConsumer.h"
#include "LegacyAudioProducer.h"

#define LEGACY_DEVICE_PATH_BASE "/dev/audio/old"

LegacyMediaAddOn::LegacyMediaAddOn( image_id imid )
	: BMediaAddOn( imid )
{
	fInitStatus = B_NO_INIT;

	consumer = new LegacyAudioConsumer( this, "ymf744/1", 0 );
	//producer = new LegacyAudioProducer( "maestro2/1" );

	fInitStatus = B_OK;
}


LegacyMediaAddOn::~LegacyMediaAddOn()
{
	delete consumer;
	//delete producer;
}


status_t
LegacyMediaAddOn::InitCheck( const char **out_failure_text )
{
	return fInitStatus;
}


int32
LegacyMediaAddOn::CountFlavors()
{
	return 0;
}


status_t
LegacyMediaAddOn::GetFlavorAt( int32 n, const flavor_info **out_info )
{
	return B_ERROR;
}


BMediaNode *
LegacyMediaAddOn::InstantiateNodeFor( const flavor_info *info, BMessage *config, status_t *out_error )
{
	return NULL;
}

status_t
LegacyMediaAddOn::RecursiveScanForDevices(const char *path)
{
	BDirectory d(path ? path : LEGACY_DEVICE_PATH_BASE);
	if (d.InitCheck() < B_OK)
		return d.InitCheck();
	BEntry ent;
	while (d.GetNextEntry(&ent) == B_OK) {
		struct stat st;
		char name[B_FILE_NAME_LENGTH];
		ent.GetName(name);
		if (d.GetStatFor(name, &st) < 0)
			continue;
		BPath p(&ent);
		// we're only interested in folders...
		if (S_ISDIR(st.st_mode)) {
			RecursiveScanForDevices(p.Path());
			// discard error
			continue;
		}
		// and (char) devices
		if (!S_ISCHR(st.st_mode))
			continue;
		
		// we want relative path
		BString s(p.Path());
		s.RemoveFirst(LEGACY_DEVICE_PATH_BASE"/");
		
		// XXX: should check first for consumer or producer...
		// XXX: use a struct/class
		fConsumers.AddItem((void *)s.String());
		//XXX: only cons for now
		//fProducers.AddItem(s.String());
	}
	return B_OK;
}

BMediaAddOn *
make_media_addon( image_id imid )
{
	return new LegacyMediaAddOn( imid );
}
