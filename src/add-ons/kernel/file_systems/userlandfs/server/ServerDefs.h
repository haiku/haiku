/*
 * Copyright 2001-2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
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

static const int32 kRequestPortSize = B_PAGE_SIZE;

}	// namespace UserlandFS

using UserlandFS::ServerSettings;
using UserlandFS::gServerSettings;
using UserlandFS::kRequestPortSize;

#endif	// USERLAND_FS_SERVER_DEFS_H
