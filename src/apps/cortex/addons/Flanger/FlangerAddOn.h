// FlangerAddOn.h
// PURPOSE
// * add-on class for FlangerNode
// HISTORY
// * e.moon		16jun99		Begun

#ifndef __FlangerAddOn_H__
#define __FlangerAddOn_H__

#include <MediaAddOn.h>

// -------------------------------------------------------- //

class FlangerAddOn	 :
	public		BMediaAddOn {
	typedef	BMediaAddOn _inherited;
	
public:					// ctor/dtor
	virtual ~FlangerAddOn();
	explicit FlangerAddOn(image_id image);
	
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

#endif /*__FlangerAddOn_H__*/