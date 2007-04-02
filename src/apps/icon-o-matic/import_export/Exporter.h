/*
 * Copyright 2006-2007, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */
#ifndef EXPORTER_H
#define EXPORTER_H


#include <Entry.h>
#include <OS.h>

class BPositionIO;
class Document;

namespace BPrivate {
namespace Icon {

class Icon;

}	// namespace Icon
}	// namespace BPrivate

using namespace BPrivate::Icon;

class Exporter {
 public:
								Exporter();
	virtual						~Exporter();

			status_t			Export(Document* document,
									   const entry_ref& ref);

	virtual	status_t			Export(const Icon* icon,
									   BPositionIO* stream) = 0;

	virtual	const char*			MIMEType() = 0;

			void				SetSelfDestroy(bool selfDestroy);

 private:
	static	int32				_ExportThreadEntry(void* cookie);
			int32				_ExportThread();
			status_t			_Export(const Icon* icon,
										const entry_ref* docRef);

			Document*			fDocument;
			Icon*				fClonedIcon;
			entry_ref			fRef;
			thread_id			fExportThread;
			bool				fSelfDestroy;
};

#endif // EXPORTER_H
