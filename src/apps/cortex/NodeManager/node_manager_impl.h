// node_manager_impl.h
// * PURPOSE
//   Helper classes & functions used by NodeManager,
//   NodeGroup, and NodeRef.
// * HISTORY
//   e.moon		10jul99		Begun

#ifndef __node_manager_impl_H__
#define __node_manager_impl_H__

#include "Connection.h"

#include "ILockable.h"

#include "cortex_defs.h"
__BEGIN_CORTEX_NAMESPACE

inline void assert_locked(const ILockable* target) {
	ASSERT(target);
	ASSERT(target->isLocked());
}
//
//// functor: disconnect Connection
//
//class disconnectFn { public:
//	NodeManager* manager;
//	disconnectFn(NodeManager* _manager) : manager(_manager) {}
//	void operator()(Connection* c) {
//		ASSERT(c);
//		ASSERT(c->isValid());
//		
//		status_t err = manager->disconnect(c);
//
//#if DEBUG
//		if(err < B_OK)
//			PRINT((
//				"* disconnect():\n"
//				"  output %s:%s\n"
//				"  input  %s:%s\n"
//				"  error '%s'\n\n",
//				c->sourceNode()->name(), c->outputName(),
//				c->destinationNode()->name(), c->inputName(),
//				strerror(err)));
//#endif
//	}
//};
//
//// functor: release NodeRef or NodeGroup (by pointer)
//// +++++ currently no error checking; however, the only error
////       release() is allowed to return is that the object has
////       already been released.
//
//template <class T>
//class releaseFn { public:
//	void operator()(T* object) {
//		ASSERT(object);
//		object->release();
//	}
//};

__END_CORTEX_NAMESPACE
#endif /*__node_manager_impl_H__*/

