// DiagramDefs.h (Cortex/DiagramView)
//
// * PURPOSE
//   Contains some global constants used by most of the
//   DiagramView classes, currently only message "what"s
//
// * HISTORY
//   c.lenz		25sep99		Begun
//

#ifndef __DiagramDefs_H__
#define __DiagramDefs_H__

#include "cortex_defs.h"
__BEGIN_CORTEX_NAMESPACE

// Message constants
enum message_t
{
	// Selection changed
	// - (DiagramItem *)		"item"
	// - (bool)					"replace"
	M_SELECTION_CHANGED		= DiagramView_message_base,

	// Diagram Box dragged
	// - (DiagramBox *)			"item"
	// - (BPoint)				"offset"
	M_BOX_DRAGGED,

	// Diagram Wire dragged
	// - (DiagramEndPoint *)	"from"
	M_WIRE_DRAGGED,

	// Diagram Wire dropped
	// - (DiagramEndPoint *)	"from"
	// - (DiagramEndPoint *)	"to"
	// - (bool)					"success"
	M_WIRE_DROPPED,

	// Diagram View 'Rect Tracking'
	// - (BPoint)				"origin"
	M_RECT_TRACKING
};

__END_CORTEX_NAMESPACE
#endif /* __DiagramDefs_H__ */
