/*
 * Copyright 2006-2010, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef NET_BUFFER_UTILITIES_H
#define NET_BUFFER_UTILITIES_H


#include <net_buffer.h>


extern net_buffer_module_info* gBufferModule;


class NetBufferModuleGetter {
	public:
		static net_buffer_module_info *Get() { return gBufferModule; }
};


//! A class to access a field safely across node boundaries
template<typename Type, int Offset, typename Module = NetBufferModuleGetter>
class NetBufferFieldReader {
public:
	NetBufferFieldReader(net_buffer* buffer)
		:
		fBuffer(buffer)
	{
		fStatus = Module::Get()->direct_access(fBuffer, Offset, sizeof(Type),
			(void**)&fData);
		if (fStatus != B_OK) {
			fStatus = Module::Get()->read(fBuffer, Offset, &fDataBuffer,
				sizeof(Type));
			fData = &fDataBuffer;
		}
	}

	status_t Status() const
	{
		return fStatus;
	}

	Type& Data() const
	{
		return *fData;
	}

	Type* operator->() const
	{
		return fData;
	}

	Type& operator*() const
	{
		return *fData;
	}

	void Sync()
	{
		if (fBuffer == NULL || fStatus < B_OK)
			return;

		if (fData == &fDataBuffer)
			Module::Get()->write(fBuffer, Offset, fData, sizeof(Type));

		fBuffer = NULL;
	}

protected:
	NetBufferFieldReader()
	{
	}

	net_buffer*	fBuffer;
	status_t	fStatus;
	Type*		fData;
	Type		fDataBuffer;
};


//! Writes back any changed data on destruction
template<typename Type, int Offset, typename Module = NetBufferModuleGetter>
class NetBufferField : public NetBufferFieldReader<Type, Offset, Module> {
public:
	NetBufferField(net_buffer* buffer)
		:
		NetBufferFieldReader<Type, Offset, Module>(buffer)
	{
	}

	~NetBufferField()
	{
		// Note, "this->" is needed here for GCC4
		this->Sync();
	}
};


//! Can remove the header from the buffer
template<typename Type, typename Module = NetBufferModuleGetter>
class NetBufferHeaderReader : public NetBufferFieldReader<Type, 0, Module> {
public:
	NetBufferHeaderReader(net_buffer* buffer)
		:
		NetBufferFieldReader<Type, 0, Module>(buffer)
	{
	}

	void Remove()
	{
		Remove(sizeof(Type));
	}

	void Remove(size_t bytes)
	{
		if (this->fBuffer != NULL) {
			Module::Get()->remove_header(this->fBuffer, bytes);
			this->fBuffer = NULL;
		}
	}
};


//!	Removes the header on destruction
template<typename Type, typename Module = NetBufferModuleGetter>
class NetBufferHeaderRemover : public NetBufferHeaderReader<Type, Module> {
public:
	NetBufferHeaderRemover(net_buffer* buffer)
		:
		NetBufferHeaderReader<Type, Module>(buffer)
	{
	}

	~NetBufferHeaderRemover()
	{
		this->Remove();
	}
};


//! A class to add a header to a buffer, syncs itself on destruction
template<typename Type, typename Module = NetBufferModuleGetter>
class NetBufferPrepend : public NetBufferFieldReader<Type, 0, Module> {
public:
	NetBufferPrepend(net_buffer* buffer, size_t size = sizeof(Type))
	{
		this->fBuffer = buffer;

		this->fStatus = Module::Get()->prepend_size(buffer, size,
			(void**)&this->fData);
		if (this->fStatus == B_OK && this->fData == NULL)
			this->fData = &this->fDataBuffer;
	}

	~NetBufferPrepend()
	{
		this->Sync();
	}
};


#endif	// NET_BUFFER_UTILITIES_H
