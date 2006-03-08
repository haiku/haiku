// ProfileBlock.h
// e.moon 19may99
//
// Quick'n'dirty profiling tool: create a ProfileBlock on the
// stack, giving it a name and a ProfileTarget to reference.
// Automatically writes an entry in the target once the
// object goes out of scope.

#ifndef __PROFILEBLOCK_H__
#define __PROFILEBLOCK_H__

#include "ProfileTarget.h"
#include <KernelKit.h>

#include "cortex_defs.h"
__BEGIN_CORTEX_NAMESPACE

class ProfileBlock {
public:					// ctor/dtor
	ProfileBlock(ProfileTarget& target, const char* name) :
		m_target(target),
		m_name(name),
		m_initTime(system_time()) {}
		
	~ProfileBlock() {
		m_target.addBlockEntry(m_name, system_time()-m_initTime);
	}
	
private:
	ProfileTarget&		m_target;
	const char* const		m_name;
	bigtime_t					m_initTime;
};

__END_CORTEX_NAMESPACE
#endif /* __PROFILEBLOCK_H__ */
