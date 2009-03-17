/*
 * Copyright 2001-2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef USERLAND_FS_PORT_H
#define USERLAND_FS_PORT_H

#include <OS.h>

class KernelDebug;

namespace UserlandFSUtil {

struct PortInfo {
};

class Port {
public:
			struct Info {
				port_id			owner_port;
				port_id			client_port;
				int32			size;
			};

public:
								Port(int32 size);
								Port(const Info* info);
								~Port();

			void				Close();

			status_t			InitCheck() const;

			const Info*			GetInfo() const;

			void*				GetBuffer() const	{ return fBuffer; }
			int32				GetCapacity() const	{ return fCapacity; }

			void				Reserve(int32 endOffset);
			void				Unreserve(int32 endOffset);
			int32				ReservedSize() const { return fReservedSize; }

			status_t			Send(const void* message, int32 size);
			status_t			Receive(void** _message, size_t* _size,
									bigtime_t timeout = -1);

private:
			friend class ::KernelDebug;

			Info				fInfo;
			uint8*				fBuffer;
			int32				fCapacity;
			int32				fReservedSize;
			status_t			fInitStatus;
			bool				fOwner;
};

}	// namespace UserlandFSUtil

using UserlandFSUtil::PortInfo;
using UserlandFSUtil::Port;

#endif	// USERLAND_FS_PORT_H
