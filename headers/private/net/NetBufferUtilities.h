/*
 * Copyright 2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef NET_BUFFER_UTILITIES_H
#define NET_BUFFER_UTILITIES_H


#include <net_buffer.h>


extern net_buffer_module_info *sBufferModule;

class NetBufferModuleGetter {
	public:
		static net_buffer_module_info *Get() { return sBufferModule; }
};

//! A class to retrieve and remove a header from a buffer
template<typename Type, typename Module = NetBufferModuleGetter > class NetBufferHeader {
	public:
		NetBufferHeader(net_buffer *buffer)
			:
			fBuffer(buffer)
		{
		}

		~NetBufferHeader()
		{
			Remove();
		}

		status_t
		Status()
		{
			return fBuffer->size < sizeof(Type) ? B_BAD_VALUE : B_OK;
		}

		Type &
		Data()
		{
			Type *data;
			if (Module::Get()->direct_access(fBuffer, 0, sizeof(Type),
					(void **)&data) == B_OK)
				return *data;

			Module::Get()->read(fBuffer, 0, &fDataBuffer, sizeof(Type));
			return fDataBuffer;
		}

		void
		Remove()
		{
			if (fBuffer != NULL) {
				Module::Get()->remove_header(fBuffer, sizeof(Type));
				fBuffer = NULL;
			}
		}

		void
		Remove(size_t bytes)
		{
			if (fBuffer != NULL) {
				Module::Get()->remove_header(fBuffer, bytes);
				fBuffer = NULL;
			}
		}

		void
		Detach()
		{
			fBuffer = NULL;
		}

	private:
		net_buffer	*fBuffer;
		Type		fDataBuffer;
};

//! A class to add a header to a buffer
template<typename Type, typename Module = NetBufferModuleGetter > class NetBufferPrepend {
	public:
		NetBufferPrepend(net_buffer *buffer)
			:
			fBuffer(buffer),
			fData(NULL)
		{
			fStatus = Module::Get()->prepend_size(buffer, sizeof(Type), (void **)&fData);
		}

		~NetBufferPrepend()
		{
			if (fBuffer != NULL)
				Detach();
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

		// TODO: I'm not sure it's a good idea to have Detach() routines
		//	in NetBufferHeader and here with such a different outcome...
		void
		Detach()
		{
			if (fData == NULL)
				Module::Get()->write(fBuffer, 0, &fDataBuffer, sizeof(Type));
			fBuffer = NULL;
		}

	private:
		net_buffer	*fBuffer;
		status_t	fStatus;
		Type		*fData;
		Type		fDataBuffer;
};

#endif	// NET_BUFFER_UTILITIES_H
