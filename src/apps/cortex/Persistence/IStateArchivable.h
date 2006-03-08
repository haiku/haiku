// IStateArchivable.h
// * PURPOSE
//   Similar to BArchivable, but provides for archiving of
//   a particular object's state, not the object itself.
//   This mechanism doesn't instantiate objects.
//
// * HISTORY
//   e.moon			27nov99		Begun

#ifndef __IStateArchivable_H__
#define __IStateArchivable_H__

#include "cortex_defs.h"
__BEGIN_CORTEX_NAMESPACE

class IStateArchivable {
public:
	// empty virtual dtor
	virtual ~IStateArchivable() {}

public:												// *** INTERFACE
	virtual status_t importState(
		const BMessage*						archive) =0;
	
	virtual status_t exportState(
		BMessage*									archive) const =0;
};

__END_CORTEX_NAMESPACE
#endif /*__IStateArchivable_H__*/