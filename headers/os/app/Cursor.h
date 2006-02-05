/*
 * Copyright 2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _CURSOR_H
#define _CURSOR_H


#include <Archivable.h>
#include <InterfaceDefs.h>


class BCursor : BArchivable {
	public:
		BCursor(const void* cursorData);
		BCursor(BMessage* data);
		virtual	~BCursor();

		virtual	status_t	Archive(BMessage* archive, bool deep = true) const;
		static BArchivable*	Instantiate(BMessage* archive);

	private:
		virtual status_t	Perform(perform_code d, void* arg);

		virtual	void		_ReservedCursor1();
		virtual	void		_ReservedCursor2();
		virtual	void		_ReservedCursor3();
		virtual	void		_ReservedCursor4();

	private:
		friend class BApplication;
		friend class BView;

		int32				fServerToken;
		bool				fNeedToFree;

		uint32				_reserved[6];
};

#endif	// _CURSOR_H
