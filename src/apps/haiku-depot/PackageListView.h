/*
 * Copyright 2013, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef PACKAGE_LIST_VIEW_H
#define PACKAGE_LIST_VIEW_H


#include <ColumnListView.h>
#include <ColumnTypes.h>


// A field type displaying both a bitmap and a string so that the
// tree display looks nicer (both text and bitmap are indented)
// TODO: Code-duplication with DriveSetup PartitionList.h
class BBitmapStringField : public BStringField {
	typedef BStringField Inherited;
public:
								BBitmapStringField(BBitmap* bitmap,
									const char* string);
	virtual						~BBitmapStringField();

			void				SetBitmap(BBitmap* bitmap);
			const BBitmap*		Bitmap() const
									{ return fBitmap; }

private:
			BBitmap*			fBitmap;
};


// BColumn for PackageListView which knows how to render
// a BBitmapStringField
// TODO: Code-duplication with DriveSetup PartitionList.h
class PackageColumn : public BTitledColumn {
	typedef BTitledColumn Inherited;
public:
								PackageColumn(const char* title,
									float width, float minWidth,
									float maxWidth, uint32 truncateMode,
									alignment align = B_ALIGN_LEFT);

	virtual	void				DrawField(BField* field, BRect rect,
									BView* parent);
	virtual float				GetPreferredWidth(BField* field,
									BView* parent) const;

	virtual	bool				AcceptsField(const BField* field) const;

	static	void				InitTextMargin(BView* parent);

private:
			uint32				fTruncateMode;
	static	float				sTextMargin;
};


class PackageListView : public BColumnListView {
public:
								PackageListView();
	virtual						~PackageListView();

	virtual void				AttachedToWindow();
	virtual	void				MessageReceived(BMessage* message);
};

#endif // PACKAGE_LIST_VIEW_H
