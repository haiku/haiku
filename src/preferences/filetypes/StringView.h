/*
 * Copyright 2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef STRING_VIEW_H
#define STRING_VIEW_H


class BLayoutItem;
class BGroupView;
class BStringView;

class StringView {
	public:
		StringView(const char* label,
			const char* text);

		void SetEnabled(bool enabled);

		void SetLabel(const char* label);
		const char* Label() const;
		void SetText(const char* text);
		const char* Text() const;

		BLayoutItem* GetLabelLayoutItem();
		BView* LabelView();
		BLayoutItem* GetTextLayoutItem();
		BView* TextView();

		operator BView*();

	private:

		BGroupView*		fView;
		BStringView*	fLabel;
		BLayoutItem*	fLabelItem;
		BStringView*	fText;
		BLayoutItem*	fTextItem;
};


#endif	// STRING_VIEW_H
