//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------

#include <AppMisc.h>
#include <Looper.h>
#include <Messenger.h>
#include <NodeMonitor.h>

#include <syscalls.h>

// TODO: Tests!


// watch_node
/*!	\brief Subscribes a target to node and/or mount watching, or unsubscribes
		   it from node watching.

	Depending of \a flags the action performed by this function varies:
	- \a flags is \c 0: The target is unsubscribed from watching the node.
	  \a node must not be \c NULL in this case.
	- \a flags contains \c B_WATCH_MOUNT: The target is subscribed to mount
	  watching.
	- \a flags contains at least one of \c B_WATCH_NAME, \c B_WATCH_STAT,
	  \c B_WATCH_ATTR, or \c B_WATCH_DIRECTORY: The target is subscribed to
	  watching the specified aspects of the node. \a node must not be \c NULL
	  in this case.

	Note, that the latter two cases are not mutual exlusive, i.e. mount and
	node watching can be requested with a single call.

	\param node node_ref referring to the node to be watched. May be \c NULL,
		   if only mount watching is requested.
	\param flags Flags indicating the actions to be performed.
	\param target Messenger referring to the target. Must be valid.
	\return \c B_OK, if everything went fine, another error code otherwise.
*/
status_t
watch_node(const node_ref *node, uint32 flags, BMessenger target)
{
	status_t error = (target.IsValid() ? B_OK : B_BAD_VALUE);
	if (error == B_OK) {
		BLooper *looper = NULL;
		BHandler *handler = target.Target(&looper);
		error = watch_node(node, flags, handler, looper);
	}
	return error;
}

// watch_node
/*!	\brief Subscribes a target to node and/or mount watching, or unsubscribes
		   it from node watching.

	Depending of \a flags the action performed by this function varies:
	- \a flags is \c 0: The target is unsubscribed from watching the node.
	  \a node must not be \c NULL in this case.
	- \a flags contains \c B_WATCH_MOUNT: The target is subscribed to mount
	  watching.
	- \a flags contains at least one of \c B_WATCH_NAME, \c B_WATCH_STAT,
	  \c B_WATCH_ATTR, or \c B_WATCH_DIRECTORY: The target is subscribed to
	  watching the specified aspects of the node. \a node must not be \c NULL
	  in this case.

	Note, that the latter two cases are not mutual exlusive, i.e. mount and
	node watching can be requested with a single call.

	\param node node_ref referring to the node to be watched. May be \c NULL,
		   if only mount watching is requested.
	\param flags Flags indicating the actions to be performed.
	\param handler The target handler. May be \c NULL, if \a looper is not
		   \c NULL. Then the preferred handler of the looper is targeted.
	\param looper The target looper. May be \c NULL, if \a handler is not
		   \c NULL. Then the handler's looper is the target looper.
	\return \c B_OK, if everything went fine, another error code otherwise.
*/
status_t
watch_node(const node_ref *node, uint32 flags, const BHandler *handler,
		   const BLooper *looper)
{
	status_t error = B_OK;
	// check looper and handler and get the handler token
	int32 handlerToken = -2;
	if (handler) {
		handlerToken = _get_object_token_(handler);
		if (looper) {
			if (looper != handler->Looper())
				error = B_BAD_VALUE;
		} else {
			looper = handler->Looper();
			if (!looper)
				error = B_BAD_VALUE;
		}
	} else if (!looper)
		error = B_BAD_VALUE;
	if (error == B_OK) {
		port_id port = _get_looper_port_(looper);
		if (flags == B_STOP_WATCHING) {
			// unsubscribe from node node watching
			if (node)
				error = _kern_stop_watching(node->device, node->node, flags, port, handlerToken);
			else
				error = B_BAD_VALUE;
		} else {
			// subscribe to...
			// mount watching
			if (flags & B_WATCH_MOUNT) {
				error = _kern_start_watching((dev_t)-1, (ino_t)-1, 0, port, handlerToken);
				flags &= ~B_WATCH_MOUNT;
			}
			// node watching
			if (error == B_OK && flags)
				error = _kern_start_watching(node->device, node->node, flags, port, handlerToken);
		}
	}
	return error;
}

// stop_watching
/*!	\brief Unsubscribes a target from node and mount monitoring.
	\param target Messenger referring to the target. Must be valid.
	\return \c B_OK, if everything went fine, another error code otherwise.
*/
status_t
stop_watching(BMessenger target)
{
	status_t error = (target.IsValid() ? B_OK : B_BAD_VALUE);
	if (error == B_OK) {
		BLooper *looper = NULL;
		BHandler *handler = target.Target(&looper);
		error = stop_watching(handler, looper);
	}
	return error;
}

// stop_watching
/*!	\brief Unsubscribes a target from node and mount monitoring.
	\param handler The target handler. May be \c NULL, if \a looper is not
		   \c NULL. Then the preferred handler of the looper is targeted.
	\param looper The target looper. May be \c NULL, if \a handler is not
		   \c NULL. Then the handler's looper is the target looper.
	\return \c B_OK, if everything went fine, another error code otherwise.
*/
status_t
stop_watching(const BHandler *handler, const BLooper *looper)
{
	status_t error = B_OK;
	// check looper and handler and get the handler token
	int32 handlerToken = -2;
	if (handler) {
		handlerToken = _get_object_token_(handler);
		if (looper) {
			if (looper != handler->Looper())
				error = B_BAD_VALUE;
		} else {
			looper = handler->Looper();
			if (!looper)
				error = B_BAD_VALUE;
		}
	} else if (!looper)
		error = B_BAD_VALUE;
	// unsubscribe
	if (error == B_OK) {
		port_id port = _get_looper_port_(looper);
		error = _kern_stop_notifying(port, handlerToken);
	}
	return error;
}

