// DormantNodeView.h
// c.lenz 22oct99
//
// RESPONSIBILITIES
// - simple extension of BListView to support
//	 drag & drop
//
// HISTORY
//   c.lenz		22oct99		Begun
//   c.lenz     27oct99		Added ToolTip support

#ifndef __DormantNodeView_H__
#define __DormantNodeView_H__

// Interface Kit
#include <ListView.h>

#include "cortex_defs.h"
__BEGIN_CORTEX_NAMESPACE

class DormantNodeView :
	public	BListView {
	typedef	BListView _inherited;
	
public:					// *** messages

	enum message_t {
		// OUTBOUND:
		// B_RAW_TYPE "which" dormant_node_info
		M_INSTANTIATE_NODE				= 'dNV0'
	};
	
public:					// *** ctor/dtor

						DormantNodeView(
							BRect frame,
							const char *name,
							uint32 resizeMode);

	virtual				~DormantNodeView();
		
public:					// *** BListView impl.

	virtual void		AttachedToWindow();
	
	virtual void		DetachedFromWindow();

	virtual void		GetPreferredSize(
							float* width,
							float* height);

	virtual void		MessageReceived(
							BMessage *message);

	virtual void		MouseDown(
							BPoint point);

	virtual void		MouseMoved(
							BPoint point,
							uint32 transit,
							const BMessage *message);

	virtual bool		InitiateDrag(
							BPoint point,
							int32 index,
							bool wasSelected);

private:				// *** internal operations

	void				_populateList();

	void				_freeList();

	void				_updateList(
							int32 addOnID);

private:				// *** data

	BListItem		   *m_lastItemUnder;
};

__END_CORTEX_NAMESPACE
#endif /*__DormantNodeView_H__*/
