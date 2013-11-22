/*
 * Copyright 2006-2013, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _MINE_MIME_SNIFFER_ADDON_MANAGER_H
#define _MINE_MIME_SNIFFER_ADDON_MANAGER_H


#include <List.h>
#include <Locker.h>


#include <mime/MimeSniffer.h>


class BFile;
class BMimeSnifferAddon;
class BMimeType;


namespace BPrivate {
namespace Storage {
namespace Mime {


class MimeSnifferAddonManager : public BPrivate::Storage::Mime::MimeSniffer {
private:
								MimeSnifferAddonManager();
								~MimeSnifferAddonManager();

public:
	static	MimeSnifferAddonManager* Default();
	static	status_t			CreateDefault();
	static	void				DeleteDefault();

			status_t			AddMimeSnifferAddon(BMimeSnifferAddon* addon);

	virtual	size_t				MinimalBufferSize();

	virtual	float				GuessMimeType(const char* fileName,
									BMimeType* type);
	virtual	float				GuessMimeType(BFile* file,
									const void* buffer, int32 length,
									BMimeType* type);

private:
			struct AddonReference;

			status_t			_GetAddons(AddonReference**& references,
									int32& count);
			void				_PutAddons(AddonReference** references,
									int32 count);

			static MimeSnifferAddonManager*	sManager;

			BLocker				fLock;
			BList				fAddons;
			size_t				fMinimalBufferSize;
};

}	// namespace Mime
}	// namespace Storage
}	// namespace BPrivate


#endif	// _MINE_MIME_SNIFFER_ADDON_MANAGER_H

