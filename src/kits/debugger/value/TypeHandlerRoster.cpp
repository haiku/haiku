/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2018, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */


#include "TypeHandlerRoster.h"

#include <new>

#include <AutoDeleter.h>
#include <AutoLocker.h>

#include "AddressValueNode.h"
#include "ArrayValueNode.h"
#include "CompoundValueNode.h"
#include "BListTypeHandler.h"
#include "BMessageTypeHandler.h"
#include "CStringTypeHandler.h"
#include "EnumerationValueNode.h"
#include "PointerToMemberValueNode.h"
#include "PrimitiveValueNode.h"
#include "Type.h"
#include "TypeHandler.h"


static int CompareTypeHandlers(const TypeHandler* a, const TypeHandler* b,
	void* state)
{
	Type* type = (Type*)state;
	return a->SupportsType(type) > b->SupportsType(type) ? 1 : -1;
}


// #pragma mark - BasicTypeHandler


namespace {


template<typename TypeClass, typename NodeClass>
class BasicTypeHandler : public TypeHandler {
public:
	virtual const char* Name() const
	{
		return "Raw";
	}

	virtual float SupportsType(Type* type) const
	{
		return dynamic_cast<TypeClass*>(type) != NULL ? 0.5f : 0;
	}

	virtual status_t CreateValueNode(ValueNodeChild* nodeChild,
		Type* type, ValueNode*& _node)
	{
		TypeClass* supportedType = dynamic_cast<TypeClass*>(type);
		if (supportedType == NULL)
			return B_BAD_VALUE;

		ValueNode* node = new(std::nothrow) NodeClass(nodeChild, supportedType);
		if (node == NULL)
			return B_NO_MEMORY;

		_node = node;
		return B_OK;
	}
};


}	// unnamed namespace


// #pragma mark - TypeHandlerRoster


/*static*/ TypeHandlerRoster* TypeHandlerRoster::sDefaultInstance = NULL;


TypeHandlerRoster::TypeHandlerRoster()
	:
	fLock("type handler roster")
{
}


TypeHandlerRoster::~TypeHandlerRoster()
{
	for (int32 i = 0; TypeHandler* handler = fTypeHandlers.ItemAt(i); i++)
		handler->ReleaseReference();
}

/*static*/ TypeHandlerRoster*
TypeHandlerRoster::Default()
{
	return sDefaultInstance;
}


/*static*/ status_t
TypeHandlerRoster::CreateDefault()
{
	if (sDefaultInstance != NULL)
		return B_OK;

	TypeHandlerRoster* roster = new(std::nothrow) TypeHandlerRoster;
	if (roster == NULL)
		return B_NO_MEMORY;
	ObjectDeleter<TypeHandlerRoster> rosterDeleter(roster);

	status_t error = roster->Init();
	if (error != B_OK)
		return error;

	error = roster->RegisterDefaultHandlers();
	if (error != B_OK)
		return error;

	sDefaultInstance = rosterDeleter.Detach();
	return B_OK;
}


/*static*/ void
TypeHandlerRoster::DeleteDefault()
{
	TypeHandlerRoster* roster = sDefaultInstance;
	sDefaultInstance = NULL;
	delete roster;
}


status_t
TypeHandlerRoster::Init()
{
	return fLock.InitCheck();
}


status_t
TypeHandlerRoster::RegisterDefaultHandlers()
{
	TypeHandler* handler;
	BReference<TypeHandler> handlerReference;

	#undef REGISTER_BASIC_HANDLER
	#define REGISTER_BASIC_HANDLER(name)						\
		handler = new(std::nothrow)								\
			BasicTypeHandler<name##Type, name##ValueNode>();	\
		handlerReference.SetTo(handler, true);					\
		if (handler == NULL || !RegisterHandler(handler))		\
			return B_NO_MEMORY;

	REGISTER_BASIC_HANDLER(Address);
	REGISTER_BASIC_HANDLER(Array);
	REGISTER_BASIC_HANDLER(Compound);
	REGISTER_BASIC_HANDLER(Enumeration);
	REGISTER_BASIC_HANDLER(PointerToMember);
	REGISTER_BASIC_HANDLER(Primitive);

	#undef REGISTER_SPECIALIZED_HANDLER
	#define REGISTER_SPECIALIZED_HANDLER(name)					\
		handler = new(std::nothrow)								\
			name##TypeHandler();								\
		handlerReference.SetTo(handler, true);					\
		if (handler == NULL || !RegisterHandler(handler))		\
			return B_NO_MEMORY;

	REGISTER_SPECIALIZED_HANDLER(CString);
	REGISTER_SPECIALIZED_HANDLER(BMessage);
	REGISTER_SPECIALIZED_HANDLER(BList);

	return B_OK;
}


int32
TypeHandlerRoster::CountTypeHandlers(Type* type)
{
	AutoLocker<BLocker>  locker(fLock);

	int32 count = 0;
	for (int32 i = 0; TypeHandler* handler = fTypeHandlers.ItemAt(i); i++) {
		if (handler->SupportsType(type) > 0)
			++count;
	}

	return count;
}


status_t
TypeHandlerRoster::FindBestTypeHandler(ValueNodeChild* nodeChild, Type* type,
	TypeHandler*& _handler)
{
	// find the best-supporting handler
	AutoLocker<BLocker> locker(fLock);

	TypeHandler* bestHandler = NULL;
	float bestSupport = 0;

	for (int32 i = 0; TypeHandler* handler = fTypeHandlers.ItemAt(i); i++) {
		float support = handler->SupportsType(type);
		if (support > 0 && support > bestSupport) {
			bestHandler = handler;
			bestSupport = support;
		}
	}

	if (bestHandler == NULL)
		return B_ENTRY_NOT_FOUND;

	bestHandler->AcquireReference();
	_handler = bestHandler;
	return B_OK;
}


status_t
TypeHandlerRoster::FindTypeHandlers(ValueNodeChild* nodeChild, Type* type,
	TypeHandlerList*& _handlers)
{
	// find the best-supporting handler
	AutoLocker<BLocker> locker(fLock);

	TypeHandlerList* handlers = new(std::nothrow) TypeHandlerList(10, false);
	ObjectDeleter<TypeHandlerList> listDeleter(handlers);
	if (handlers == NULL)
		return B_NO_MEMORY;

	for (int32 i = 0; TypeHandler* handler = fTypeHandlers.ItemAt(i); i++) {
		if (handler->SupportsType(type) > 0) {
			if (!handlers->AddItem(handler))
				return B_NO_MEMORY;
		}
	}

	if (handlers->CountItems() == 0)
		return B_ENTRY_NOT_FOUND;

	for (int32 i = 0; TypeHandler* handler = handlers->ItemAt(i); i++)
		handler->AcquireReference();

	handlers->SortItems(CompareTypeHandlers, type);

	_handlers = handlers;
	listDeleter.Detach();

	return B_OK;
}


status_t
TypeHandlerRoster::CreateValueNode(ValueNodeChild* nodeChild, Type* type,
	TypeHandler* handler, ValueNode*& _node)
{
	BReference<TypeHandler> handlerReference;

	// if the caller doesn't supply us with a handler to use, try to find
	// the best match.
	if (handler == NULL) {
		// find the best-supporting handler
		while (true) {
			status_t error = FindBestTypeHandler(nodeChild, type, handler);
			if (error == B_OK) {
				handlerReference.SetTo(handler, true);
				break;
			}

			// not found yet -- try to strip a modifier/typedef from the type
			Type* nextType = type->ResolveRawType(true);
			if (nextType == NULL || nextType == type)
				return B_UNSUPPORTED;

			type = nextType;
		}
	}

	return handler->CreateValueNode(nodeChild, type, _node);
}


bool
TypeHandlerRoster::RegisterHandler(TypeHandler* handler)
{
	if (!fTypeHandlers.AddItem(handler))
		return false;

	handler->AcquireReference();
	return true;
}


void
TypeHandlerRoster::UnregisterHandler(TypeHandler* handler)
{
	if (fTypeHandlers.RemoveItem(handler))
		handler->ReleaseReference();
}
