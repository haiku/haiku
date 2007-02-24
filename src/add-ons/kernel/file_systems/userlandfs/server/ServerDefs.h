// ServerDefs.h

#ifndef USERLAND_FS_SERVER_DEFS_H
#define USERLAND_FS_SERVER_DEFS_H

#include <OS.h>

namespace UserlandFS {

class ServerSettings {
public:
								ServerSettings();
								~ServerSettings();

			void				SetEnterDebugger(bool enterDebugger);
			bool				ShallEnterDebugger() const;

private:
			bool				fEnterDebugger;
};

extern ServerSettings gServerSettings;

enum {
	UFS_REGISTER_FS				= 'rgfs',
	UFS_REGISTER_FS_ACK			= 'rfsa',
	UFS_REGISTER_FS_DENIED		= 'rfsd',
};

extern const char* kUserlandFSDispatcherClipboardName;

static const int32 kRequestPortSize = B_PAGE_SIZE;

}	// namespace UserlandFS

using UserlandFS::ServerSettings;
using UserlandFS::gServerSettings;
using UserlandFS::kUserlandFSDispatcherClipboardName;
using UserlandFS::kRequestPortSize;

#endif	// USERLAND_FS_SERVER_DEFS_H
