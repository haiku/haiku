// AudioAdapterAddOn.h
// * PURPOSE
//   To test the IAudioOp framework, this add-on creates
//   'do-nothing' filter nodes.
//
// * TO DO
//   +++++ reflect build time in node info
//
// * HISTORY
//   e.moon		8sep99		Begun

#ifndef __AudioAdapterAddOn_H__
#define __AudioAdapterAddOn_H__

#include <MediaAddOn.h>

// -------------------------------------------------------- //

class AudioAdapterAddOn	 :
	public		BMediaAddOn {
	typedef		BMediaAddOn _inherited;
	
public:					// ctor/dtor
//	virtual ~AudioAdapterAddOn();
	explicit AudioAdapterAddOn(image_id image);

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

#endif /*__AudioAdapterAddOn_H__*/
