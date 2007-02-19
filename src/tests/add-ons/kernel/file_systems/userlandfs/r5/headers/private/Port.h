// Port.h

#ifndef USERLAND_FS_PORT_H
#define USERLAND_FS_PORT_H

#include <OS.h>

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

			void*				GetBuffer() const;
			int32				GetCapacity() const;

			void*				GetMessage() const;
			int32				GetMessageSize() const;

			status_t			Send(int32 size);
			status_t			SendAndReceive(int32 size);
			status_t			Receive(bigtime_t timeout = -1);

private:
			friend class KernelDebug;

			Info				fInfo;
			uint8*				fBuffer;
			int32				fCapacity;
			int32				fMessageSize;
			status_t			fInitStatus;
			bool				fOwner;
};

}	// namespace UserlandFSUtil

using UserlandFSUtil::PortInfo;
using UserlandFSUtil::Port;

#endif	// USERLAND_FS_PORT_H
