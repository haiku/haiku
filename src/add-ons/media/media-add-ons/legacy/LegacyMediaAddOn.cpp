#include <support/Autolock.h>
#include <media/MediaDefs.h>

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <media/MediaAddOn.h>

#include <Autolock.h>
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
	, fInternalIds( 0 )
	, fListsLock( "LegacyMediaAddOn:ListLock" )
{
	fInitStatus = B_NO_INIT;

	fMediaFormat.type = B_MEDIA_RAW_AUDIO;
	fMediaFormat.u.raw_audio = media_raw_audio_format::wildcard;

	fInitStatus = RecursiveScanForDevices();
	//consumer = new LegacyAudioConsumer( this, "ymf744/1", 0 );
	//producer = new LegacyAudioProducer( "maestro2/1" );

}


LegacyMediaAddOn::~LegacyMediaAddOn()
{
	//delete consumer;
	//delete producer;
	LegacyDevice *dev;
	while ((dev = (LegacyDevice *)fConsumers.RemoveItem((int32)0))) {
		delete dev->consumer;
		//delete dev->producer;
		delete dev;
	}
	while ((dev = (LegacyDevice *)fProducers.RemoveItem((int32)0))) {
		//delete dev->consumer;
		//delete dev->producer;
		delete dev;
	}
	
	
}


status_t
LegacyMediaAddOn::InitCheck( const char **out_failure_text )
{
	return fInitStatus;
}


int32
LegacyMediaAddOn::CountFlavors()
{
	int32 count;
	BAutolock al(fListsLock);
	
	count = fConsumers.CountItems() + fProducers.CountItems();
	return count;
}


status_t
LegacyMediaAddOn::GetFlavorAt( int32 n, const flavor_info **out_info )
{
	BAutolock al(fListsLock);
	LegacyDevice *dev;
	if (n < 0)
		return EINVAL;
	for (n = fConsumers.CountItems() - 1; n >= 0; n--) {
		dev = (LegacyDevice *)fConsumers.ItemAt(n);
		if (dev->flavor.internal_id != n)
			continue;
		*out_info = &dev->flavor;
		return B_OK;
	}
	for (n = fProducers.CountItems() - 1; n >= 0; n--) {
		dev = (LegacyDevice *)fProducers.ItemAt(n);
		if (dev->flavor.internal_id != n)
			continue;
		*out_info = &dev->flavor;
		return B_OK;
	}
	return B_ERROR;
}


BMediaNode *
LegacyMediaAddOn::InstantiateNodeFor( const flavor_info *info, BMessage *config, status_t *out_error )
{
	BAutolock al(fListsLock);
	LegacyDevice *dev;
	int32 n;
	int32 consumerCount = fConsumers.CountItems();
	for (n = consumerCount - 1; n >= 0; n--) {
		dev = (LegacyDevice *)fConsumers.ItemAt(n);
		//if (info != &dev->flavor) // XXX memcmp?
		if (info->internal_id != dev->flavor.internal_id)
			continue;
		if (dev->inuse) // EBUSY!
			return NULL;
		dev->consumer = new LegacyAudioConsumer( this, dev->name.String(), n );
		if (!dev->consumer)
			return NULL;
		dev->inuse = true;
		return dev->consumer;
	}
/*
	for (n = fProducers.CountItems() - 1; n >= 0; n--) {
		dev = (LegacyDevice *)fProducers.ItemAt(n);
		if (info != &dev->flavor) // XXX memcmp?
			continue;
		if (dev->inuse) // EBUSY!
			return NULL;
		dev->producer = new LegacyAudioProducer( this, dev->name.String(), consumerCount + n );
		if (!dev->producer)
			return NULL;
		dev->inuse = true;
		return dev->producer;
	}
*/
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
		LegacyDevice *dev = new LegacyDevice;
		dev->name = s.String();
		dev->flavor.name = (char *)/*WTF*/dev->name.String();
		dev->flavor.info = (char *)dev->name.String();
		dev->flavor.kinds = B_BUFFER_CONSUMER | /*B_CONTROLLABLE |*/ B_PHYSICAL_OUTPUT;
		dev->flavor.flavor_flags = 0; //B_FLAVOR_IS_GLOBAL;
		dev->flavor.internal_id = fInternalIds++;
		dev->flavor.possible_count = 1;
		dev->flavor.in_format_count = 1;
		dev->flavor.in_format_flags = 0;
		dev->flavor.in_formats = &fMediaFormat;
		dev->flavor.out_format_count = 0;
		dev->flavor.out_format_flags = 0;
		dev->flavor.out_formats = NULL;
		dev->inuse = false;
		dev->consumer = NULL;
		//dev->producer = NULL;
		fConsumers.AddItem((void *)dev);
		//XXX: only cons for now
		//fProducers.AddItem(s.String());
	}
	return B_OK;
}

#if 0

struct flavor_info {
        char *                          name;
        char *                          info;
        uint64                          kinds;                  /* node_kind */
        uint32                          flavor_flags;
        int32                           internal_id;    /* For BMediaAddOn internal use */
        int32                           possible_count; /* 0 for "any number" */

        int32                           in_format_count;        /* for BufferConsumer kinds */
        uint32                          in_format_flags;        /* set to 0 */
        const media_format *    in_formats;

        int32                           out_format_count;       /* for BufferProducer kinds */
        uint32                          out_format_flags;       /* set to 0 */
        const media_format *    out_formats;

        uint32                          _reserved_[16];

private:
        flavor_info & operator=(const flavor_info & other);
};
#endif


BMediaAddOn *
make_media_addon( image_id imid )
{
	return new LegacyMediaAddOn( imid );
}
