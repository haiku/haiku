#include <support/Autolock.h>
#include <media/MediaDefs.h>

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <media/MediaAddOn.h>

#include "LegacyMediaAddOn.h"

#include "LegacyAudioConsumer.h"
#include "LegacyAudioProducer.h"


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


BMediaAddOn *
make_media_addon( image_id imid )
{
	return new LegacyMediaAddOn( imid );
}
