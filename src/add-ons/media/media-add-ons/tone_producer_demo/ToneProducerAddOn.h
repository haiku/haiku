/* Written by Eric Moon, from the Cortex Source code archive.
 * Distributed under the terms of the Be Sample Code License
 */

// ToneProducerAddOn.h
// e.moon 4jun99
//
// PURPOSE
//   Quickie AddOn class for the ToneProducer node.
//

#ifndef __ToneProducerAddOn_H__
#define __ToneProducerAddOn_H__

#include <MediaAddOn.h>

// -------------------------------------------------------- //

class ToneProducerAddOn :
	public		BMediaAddOn {
	typedef	BMediaAddOn _inherited;
	
public:					// ctor/dtor
	virtual ~ToneProducerAddOn();
	explicit ToneProducerAddOn(image_id image);
	
public:					// BMediaAddOn impl
virtual	status_t InitCheck(
				const char** out_failure_text);
virtual	int32 CountFlavors();
virtual	status_t GetFlavorAt(
				int32 n,
				const flavor_info ** out_info);
virtual	BMediaNode * InstantiateNodeFor(
				const flavor_info * info,
				BMessage * config,
				status_t * out_error);
virtual	status_t GetConfigurationFor(
				BMediaNode * your_node,
				BMessage * into_message);

virtual	bool WantsAutoStart() { return false; }
virtual	status_t AutoStart(
				int in_count,
				BMediaNode ** out_node,
				int32 * out_internal_id,
				bool * out_has_more) { return B_OK; }
};

#endif /*__ToneProducerAddOn_H__*/
