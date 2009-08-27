/*
 * Copyright 2009, Haiku Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _BITMAP_STREAM_H
#define _BITMAP_STREAM_H


#include <ByteOrder.h>
#include <DataIO.h>
#include <TranslationDefs.h>
#include <TranslatorFormats.h>


class BBitmap;


class BBitmapStream : public BPositionIO {
public:
								BBitmapStream(BBitmap* bitmap = NULL);
	virtual						~BBitmapStream();

	virtual	ssize_t				ReadAt(off_t offset, void* buffer, size_t size);
	virtual	ssize_t				WriteAt(off_t offset, const void* buffer,
									size_t size);
	virtual	off_t				Seek(off_t position, uint32 seekMode);
	virtual	off_t				Position() const;
	virtual	off_t				Size() const;
	virtual	status_t			SetSize(off_t size);

			status_t			DetachBitmap(BBitmap** _bitmap);

protected:
			void				SwapHeader(const TranslatorBitmap* source,
									TranslatorBitmap* destination);

protected:
			TranslatorBitmap	fHeader;
			BBitmap*			fBitmap;
			size_t				fPosition;
			size_t				fSize;
			bool				fDetached;

private:
	virtual	void _ReservedBitmapStream1();
	virtual void _ReservedBitmapStream2();

private:
			TranslatorBitmap*	fBigEndianHeader;
			long				_reserved[5];
};


#endif	// _BITMAP_STREAM_H
