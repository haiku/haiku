// MixerAddon.h
// David Shipman, 2002
//
//   Quick addon header for the Audio Mixer
//

#ifndef __AudioMixerAddOn_H_
#define __AudioMixerAddOn_H_

#include <MediaAddOn.h>

// -------------------------------------------------------- //

class AudioMixerAddon :
	public		BMediaAddOn {
	typedef	BMediaAddOn _inherited;
	
public:					// ctor/dtor
	virtual ~AudioMixerAddon();
	explicit AudioMixerAddon(image_id image);
	
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

virtual	bool WantsAutoStart();
virtual	status_t AutoStart(
				int in_index,
				BMediaNode ** out_node,
				int32 * out_internal_id,
				bool * out_has_more);
private:
	status_t ConnectToOutput(BMediaNode *node);
				
private:
	media_format 	*fFormat;
	flavor_info		*fInfo;
};

#endif /*__AudioMixerAddOn_H_*/
