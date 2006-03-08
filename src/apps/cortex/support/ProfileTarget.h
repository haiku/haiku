// ProfileTarget.h
// e.moon 19may99
//
// A bare-bones profiling tool.

#ifndef __EMOON_PROFILETARGET_H__
#define __EMOON_PROFILETARGET_H__

#include <map>

#include <String.h>

#include "cortex_defs.h"
__BEGIN_CORTEX_NAMESPACE

class ProfileTarget {
public:					// ctor/dtor
	virtual ~ProfileTarget();
	ProfileTarget();
	
public:					// user operations
	void clear();
	void dump() const;

public:					// profile-source operations
	inline void addBlockEntry(const char* blockName, bigtime_t elapsed);

public:
	struct block_entry {
		block_entry() : elapsed(0), count(0), name(0) {}
		
		bigtime_t		elapsed;
		uint32			count;
		const char*		name;
	};
	
private:					// implementation
			
	typedef map<BString, block_entry> block_entry_map;
	block_entry_map		m_blockEntryMap;
};

//bool operator<(const ProfileTarget::block_entry& a, const ProfileTarget::block_entry& b);

__END_CORTEX_NAMESPACE
#endif /* __EMOON_PROFILETARGET_H__ */
