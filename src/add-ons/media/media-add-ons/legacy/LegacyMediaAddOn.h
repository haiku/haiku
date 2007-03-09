#ifndef _LEGACY_MEDIA_ADDON_H
#define _LEGACY_MEDIA_ADDON_H

#include <media/MediaAddOn.h>
#include <List.h>
#include <Locker.h>
#include <String.h>

#include "LegacyAudioConsumer.h"
//#include "LegacyAudioProducer.h"


class LegacyDevice {
public:
	BString name;
	flavor_info flavor;
	bool inuse;
	LegacyAudioConsumer	*consumer;
	//LegacyAudioProducer	*producer;
	
};

class LegacyMediaAddOn : public BMediaAddOn
{
	public:
							LegacyMediaAddOn( image_id imid );
		virtual				~LegacyMediaAddOn();

		virtual	status_t	InitCheck( const char **out_failure_text );

		virtual	int32		CountFlavors();
		virtual	status_t	GetFlavorAt( int32 n, const flavor_info **out_info );
		virtual	BMediaNode	*InstantiateNodeFor( const flavor_info *info, BMessage *config, status_t *out_error );

		virtual	status_t	GetConfigurationFor( BMediaNode *node, BMessage *message )
								{ return B_ERROR; }
		virtual	status_t	SaveConfigInfo( BMediaNode *node, BMessage *message )
								{ return B_OK; }

		virtual	bool		WantsAutoStart()
								{ return false; }
		virtual	status_t	AutoStart( int in_count, BMediaNode **out_node, int32 *out_internal_id, bool *out_has_more )
								{ return B_ERROR; }

		status_t			RecursiveScanForDevices(const char *path=NULL);

	private:
		status_t			fInitStatus;
		int32				fInternalIds;

		flavor_info			fFlavorInfo;
		media_format		fMediaFormat;

		BLocker				fListsLock;
		BList				fConsumers;
		BList				fProducers;

		//OLD:LegacyAudioConsumer	*consumer;
};


extern "C" _EXPORT BMediaAddOn *make_media_addon( image_id you );

#endif
