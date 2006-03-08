// DormantNodeListItem.h
// e.moon 2jun99
//
// * PURPOSE
// - represent a single dormant (add-on) media node in a
//   list view.
//
// * HISTORY
//   22oct99	c.lenz		complete rewrite
//   27oct99	c.lenz		added tooltip support

#ifndef __DormantNodeListItem_H__
#define __DormantNodeListItem_H__

// Interface Kit
#include <Bitmap.h>
#include <Font.h>
#include <ListItem.h>
// Media Kit
#include <MediaAddOn.h>

#include "cortex_defs.h"
__BEGIN_CORTEX_NAMESPACE

class DormantNodeView;
class MediaIcon;

class DormantNodeListItem :
	public	BListItem {
	typedef	BListItem _inherited;
	
public:					// *** constants

	// [e.moon 26oct99] moved definition to DormantNodeListItem.cpp
	// to appease the Metrowerks compiler.
	static const float M_ICON_H_MARGIN;
	static const float M_ICON_V_MARGIN;

public:					// *** ctor/dtor

						DormantNodeListItem(
							const dormant_node_info &nodeInfo);

	virtual				~DormantNodeListItem();

public:					// *** accessors

	const dormant_node_info &info() const 
						{ return m_info; }

public:					// *** operations

	void				mouseOver(
							BView *owner,
							BPoint point,
							uint32 transit);

	BRect				getRealFrame(
							const BFont *font) const;

	BBitmap			   *getDragBitmap();
	
	void				showContextMenu(
							BPoint point,
							BView *owner);
	
public:					// *** BListItem impl.

	virtual void		DrawItem(
							BView *owner,
							BRect frame,
							bool drawEverything = false);

	virtual void		Update(
							BView *owner,
							const BFont *fFont);
		
protected:				// *** compare functions

	friend int			compareName(
							const void *lValue,
							const void *rValue);

	friend int			compareAddOnID(
							const void *lValue,
							const void *rValue);

private:				// *** data

	dormant_node_info	m_info;

	BRect				m_frame;

	font_height			m_fontHeight;
	
	MediaIcon		   *m_icon;
};

__END_CORTEX_NAMESPACE
#endif /*__DormantNodeListItem_H__*/
