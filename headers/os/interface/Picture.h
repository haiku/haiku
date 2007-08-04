//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, OpenBeOS
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		Picture.h
//	Author:			Marc Flerackers (mflerackers@androme.be)
//	Description:	BPicture records a series of drawing instructions that can
//                  be "replayed" later.
//------------------------------------------------------------------------------

#ifndef	_PICTURE_H
#define	_PICTURE_H

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------
#include <BeBuild.h>
#include <InterfaceDefs.h>
#include <Rect.h>
#include <Archivable.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

class BView;
struct _BPictureExtent_;

// BPicture class --------------------------------------------------------------
class BPicture : public BArchivable {
public:
							BPicture();
							BPicture(const BPicture &original);
							BPicture(BMessage *data);
virtual						~BPicture();
static	BArchivable			*Instantiate(BMessage *data);
virtual	status_t			Archive(BMessage *data, bool deep = true) const;
virtual	status_t			Perform(perform_code d, void *arg);

		status_t			Play(void **callBackTable,
								int32 tableEntries,
								void *userData);

		status_t			Flatten(BDataIO *stream);
		status_t			Unflatten(BDataIO *stream);

/*----- Private or reserved -----------------------------------------*/
private:

friend class BWindow;
friend class BView;
friend class BPrintJob;

virtual	void				_ReservedPicture1();
virtual	void				_ReservedPicture2();
virtual	void				_ReservedPicture3();

		BPicture			&operator=(const BPicture &);

		void				init_data();
		void				import_data(const void *data, int32 size, BPicture **subs, int32 subCount);
		void				import_old_data(const void *data, int32 size);
		void				set_token(int32 token);
		bool				assert_local_copy();
		bool				assert_old_local_copy();
		bool				assert_server_copy();

		/**Deprecated API**/
							BPicture(const void *data, int32 size);
		const void			*Data() const;
		int32				DataSize() const;

		void				usurp(BPicture *lameDuck);
		BPicture			*step_down();

		int32				token;
		_BPictureExtent_	*extent;
		BPicture			*usurped;
		uint32				_reserved[3];
};
//------------------------------------------------------------------------------

#endif // _PICTURE_H

