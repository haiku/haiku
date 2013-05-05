/*
 * Copyright 2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Mikael Konradson, mikael.konradson@gmail.com
 */
#ifndef FONT_DEMO_VIEW_H
#define FONT_DEMO_VIEW_H


#include <View.h>
#include <Region.h>
#include <String.h>

class BShape;
class BBitmap;


class FontDemoView : public BView {
	public:
		FontDemoView(BRect rect);
		virtual ~FontDemoView();

		virtual void Draw(BRect updateRect);
		virtual void MessageReceived(BMessage* message);
		virtual void FrameResized(float width, float height);

		virtual	void SetFontSize(float size);
		const float FontSize() const { return fFont.Size(); }

		void SetDrawBoundingBoxes(bool state);
		bool BoundingBoxes() const { return fBoundingBoxes; }

		void SetFontShear(float shear);
		const float Shear() const { return fFont.Shear(); }

		void SetFontRotation(float rotation);
		const float Rotation() const { return fFont.Rotation(); }

		void SetString(BString string);
		BString String() const;

		void SetAntialiasing(bool state);

		void SetSpacing(float space);
		const float Spacing() const	{ return fSpacing; }

		void SetOutlineLevel(int8 outline);
		const int8 OutLineLevel() const { return fOutLineLevel; }

	private:
		void _AddShapes(BString string);
		void _DrawView(BView* view);

		BView* _GetView(BRect rect);
		void _NewBitmap(BRect rect);

		BBitmap*	fBitmap;
		BView*		fBufferView;

		BString		fString;
		float		fFontSize;
		float 		fSpacing;
		int8		fOutLineLevel;
		drawing_mode	fDrawingMode;

		BRegion 	fBoxRegion;
		BFont 		fFont;
		bool 		fBoundingBoxes;
		bool 		fDrawShapes;
		BShape**	fShapes;
};

#endif	// FONT_DEMO_VIEW_H


