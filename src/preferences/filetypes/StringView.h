/*
 * Copyright 2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef STRING_VIEW_H
#define STRING_VIEW_H


#include <String.h>
#include <View.h>


class StringView : public BView {
	public:
		StringView(BRect frame, const char* name, const char* label,
			const char* text, uint32 resizeMask = B_FOLLOW_LEFT | B_FOLLOW_TOP,
			uint32 flags = B_WILL_DRAW | B_FRAME_EVENTS);
		virtual ~StringView();

		virtual void Draw(BRect updateRect);
		virtual void AttachedToWindow();

		virtual void FrameResized(float width, float height);

		virtual void GetPreferredSize(float* _width, float* _height);
		virtual void ResizeToPreferred();

		void SetEnabled(bool enabled);
		bool IsEnabled() const { return fEnabled; }

		void SetLabel(const char* label);
		const char* Label() const { return fLabel.String(); }

		void SetText(const char* text);
		const char* Text() const { return fText.String(); }

		void SetDivider(float divider);
		float Divider() const { return fDivider; }

		void SetAlignment(alignment labelAlignment, alignment textAlignment);
		void GetAlignment(alignment* _label, alignment* _text) const;

	private:
		void _UpdateText();

		BString		fLabel;
		BString 	fText;
		BString 	fTruncatedText;
		float		fDivider;
		alignment	fLabelAlignment;
		alignment	fTextAlignment;
		bool		fEnabled;
};

#endif	// STRING_VIEW_H
