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

