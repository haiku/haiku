/*
 * Copyright (c) 1999-2000, Eric Moon.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions, and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF TITLE, NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


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
struct dormant_flavor_info;

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
