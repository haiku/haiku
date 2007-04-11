/*
 * Copyright 2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef NET_BUFFER_UTILITIES_H
#define NET_BUFFER_UTILITIES_H


#include <net_buffer.h>


extern net_buffer_module_info *gBufferModule;

class NetBufferModuleGetter {
	public:
		static net_buffer_module_info *Get() { return gBufferModule; }
};

//! A class to access a field safely across node boundaries
template<typename Type, int Offset, typename Module = NetBufferModuleGetter>
class NetBufferFieldReader {
	public:
		NetBufferFieldReader(net_buffer *buffer)
			:
			fBuffer(buffer),
			fStatus(B_BAD_VALUE)
		{
			if ((Offset + sizeof(Type)) <= buffer->size) {
				fStatus = Module::Get()->direct_access(fBuffer, Offset,
					sizeof(Type), (void **)&fData);
				if (fStatus != B_OK) {
					fData = NULL;
					fStatus = Module::Get()->read(fBuffer, Offset,
						&fDataBuffer, sizeof(Type));
				}
			}
		}

		status_t
		Status()
		{
			return fStatus;
		}

		Type &
		Data()
		{
			if (fData != NULL)
				return *fData;

			return fDataBuffer;
		}

		Type *
		operator->()
		{
			return &Data();
		}

		Type &
		operator*()
		{
			return Data();
		}

		void
		Sync()
		{
			if (fBuffer == NULL)
				return;

			if (fData == NULL)
				Module::Get()->write(fBuffer, Offset, &fDataBuffer,
					sizeof(Type));

			fBuffer = NULL;
		}

	protected:
		NetBufferFieldReader() {}

		net_buffer	*fBuffer;
		status_t	fStatus;
		Type		*fData;
		Type		fDataBuffer;
};

template<typename Type, int Offset, typename Module = NetBufferModuleGetter>
class NetBufferField : public NetBufferFieldReader<Type, Offset, Module> {
	public:
		NetBufferField(net_buffer *buffer)
			: NetBufferFieldReader<Type, Offset, Module>(buffer)
		{}

		~NetBufferField()
		{
			Sync();
		}
};

template<typename Type, typename Module = NetBufferModuleGetter>
class NetBufferHeaderReader : public NetBufferFieldReader<Type, 0, Module> {
	public:
		NetBufferHeaderReader(net_buffer *buffer)
			: NetBufferFieldReader<Type, 0, Module>(buffer)
		{}

		void
		Remove()
		{
			Remove(sizeof(Type));
		}

		void
		Remove(size_t bytes)
		{
			if (fBuffer != NULL) {
				Module::Get()->remove_header(fBuffer, bytes);
				fBuffer = NULL;
			}
		}
};

template<typename Type, typename Module = NetBufferModuleGetter>
class NetBufferHeaderRemover : public NetBufferHeaderReader<Type, Module> {
	public:
		NetBufferHeaderRemover(net_buffer *buffer)
			: NetBufferHeaderReader<Type, Module>(buffer)
		{}

		~NetBufferHeaderRemover()
		{
			Remove();
		}
};

//! A class to add a header to a buffer
template<typename Type, typename Module = NetBufferModuleGetter>
class NetBufferPrepend : public NetBufferFieldReader<Type, 0, Module> {
	public:
		NetBufferPrepend(net_buffer *buffer, size_t size = 0)
		{
			fBuffer = buffer;
			fData = NULL;

			if (size == 0)
				size = sizeof(Type);

			fStatus = Module::Get()->prepend_size(buffer, size, (void **)&fData);
		}

		~NetBufferPrepend()
		{
			Sync();
		}
};

#endif	// NET_BUFFER_UTILITIES_H
