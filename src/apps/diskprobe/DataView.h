/* 
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/
#ifndef DATA_VIEW_H
#define DATA_VIEW_H


#include <View.h>
#include <String.h>
#include <Path.h>


class DataEditor;


class DataView : public BView {
	public:
		DataView(BRect rect, DataEditor &editor);
		virtual ~DataView();

		virtual void DetachedFromWindow();
		virtual void AttachedToWindow();
		virtual void MessageReceived(BMessage *message);
		virtual void Draw(BRect updateRect);

		virtual void SetFont(const BFont *font, uint32 properties = B_FONT_ALL);
		virtual void GetPreferredSize(float *_width, float *_height);

		void SetFontSize(float point);

	private:
		void UpdateFromEditor(BMessage *message = NULL);
		void ConvertLine(char *line, off_t offset, const uint8 *buffer, size_t size);

		DataEditor	&fEditor;
		int32		fPositionLength;
		uint8		*fData;
		uint32		fDataSize;
		off_t		fOffset;
		float		fAscent;
		int32		fFontHeight;
};

#endif	/* DATA_VIEW_H */
