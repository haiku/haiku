// MediaIcon.h (Cortex/Support)
//
// * PURPOSE
//   
//   Easy construction of an icon-representation for common 
//   kinds of nodes
//
// * NOTES
//
//	 5dec99 i've included some workarounds for the buggy GetDormantNodeFor()
//			 function
//   24oct99 simple to use but *not* very efficient!
//
// * HISTORY
//   c.lenz		17oct99		Begun
//	 c.lenz		5dec99		Bugfixes & support of application icons
//

#ifndef __MediaIcon_H__
#define __MediaIcon_H__

// Interface Kit
#include <Bitmap.h>
#include <InterfaceDefs.h>
// Media Kit
#include <MediaDefs.h>
// Storage Kit
#include <Mime.h>

class BRegion;

#include "cortex_defs.h"
__BEGIN_CORTEX_NAMESPACE

class MediaIcon :
	public BBitmap {

public:					// *** ctor/dtor

						MediaIcon(
							const live_node_info &nodeInfo,
							icon_size size);

						MediaIcon(
							const dormant_node_info &nodeInfo,
							icon_size size);

	virtual				~MediaIcon();

private:				// *** internal accessors

	// return the basic node_kind
	bool				_isPhysicalInput() const;
	bool				_isPhysicalOutput() const;
	bool				_isProducer() const;
	bool				_isFilter() const;
	bool				_isConsumer() const;

	// special cases (media-type independant)
	bool				_isSystemMixer() const;
	bool				_isFileInterface() const;
	bool				_isTimeSource() const;

private:				// *** internal operations

	void				_findIconFor(
							const live_node_info &nodeInfo);

	void				_findIconFor(
							const dormant_node_info &nodeInfo);

	void				_findDefaultIconFor(
							bool audioIn,
							bool audioOut,
							bool videoIn,
							bool videoOut);

	static void			_getMediaTypesFor(
							const live_node_info &nodeInfo,
							bool *audioIn,
							bool *audioOut,
							bool *videoIn,
							bool *videoOut);

	static void			_getMediaTypesFor(
							const dormant_flavor_info &flavorInfo,
							bool *audioIn,
							bool *audioOut,
							bool *videoIn,
							bool *videoOut);

private:				// *** data

	// the icon size: B_LARGE_ICON or B_MINI_ICON
	icon_size			m_size;

	// the node kinds (e.g. B_BUFFER_PRODUCER)
	uint32				m_nodeKind;
};

__END_CORTEX_NAMESPACE
#endif /* __MediaIcon_H__ */
