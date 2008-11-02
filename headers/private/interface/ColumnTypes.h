/*******************************************************************************
/
/	File:			ColumnTypes.h
/
/   Description:    Experimental classes that implement particular column/field
/					data types for use in BColumnListView.
/
/	Copyright 2000+, Be Incorporated, All Rights Reserved
/
*******************************************************************************/


#ifndef _COLUMN_TYPES_H
#define _COLUMN_TYPES_H

#include "ColumnListView.h"
#include <String.h>
#include <Font.h>
#include <Bitmap.h>


//=====================================================================
// Common base-class: a column that draws a standard title at its top.

class BTitledColumn : public BColumn
{
	public:
							BTitledColumn		(const char *title,
												 float width,
												 float minWidth,
												 float maxWidth,
												 alignment align = B_ALIGN_LEFT);
		virtual void		DrawTitle			(BRect rect,
												 BView* parent);
		virtual void		GetColumnName		(BString* into) const;

		void				DrawString			(const char*,
												 BView*,
												 BRect);
		void				SetTitle			(const char* title);
		void				Title				(BString* forTitle) const; // sets the BString arg to be the title
		float				FontHeight			() const;

		virtual float		GetPreferredWidth(BField* field, BView* parent) const;

	private:
		float				fFontHeight;
		BString				fTitle;
};


//=====================================================================
// Field and column classes for strings.

class BStringField : public BField
{
	public:
									BStringField		(const char* string);

				void				SetString			(const char* string);
				const char*			String				() const;
				void				SetClippedString	(const char* string);
				const char*			ClippedString		();
				void				SetWidth			(float);
				float				Width				();
	
	private:
		float				fWidth;
		BString				fString;
		BString				fClippedString;
};


//--------------------------------------------------------------------

class BStringColumn : public BTitledColumn
{
	public:
							BStringColumn		(const char *title,
												 float width,
												 float minWidth,
												 float maxWidth,
												 uint32 truncate,
												 alignment align = B_ALIGN_LEFT);
		virtual void		DrawField			(BField* field,
												 BRect rect,
												 BView* parent);
		virtual int			CompareFields		(BField* field1,
												 BField* field2);
		virtual float		GetPreferredWidth(BField* field, BView* parent) const;
		virtual	bool		AcceptsField        (const BField* field) const;

	private:
		uint32				fTruncate;
};


//=====================================================================
// Field and column classes for dates.

class BDateField : public BField
{
	public:
							BDateField			(time_t* t);
		void				SetWidth			(float);
		float				Width				();
		void				SetClippedString	(const char*);
		const char*			ClippedString		();
		time_t				Seconds				();
		time_t				UnixTime			();

	private:	
		struct tm			fTime;
		time_t				fUnixTime;
		time_t				fSeconds;
		BString				fClippedString;
		float				fWidth;
};


//--------------------------------------------------------------------

class BDateColumn : public BTitledColumn
{
	public:
							BDateColumn			(const char* title,
												 float width,
												 float minWidth,
												 float maxWidth,
												 alignment align = B_ALIGN_LEFT);
		virtual void		DrawField			(BField* field,
												 BRect rect,
												 BView* parent);
		virtual int			CompareFields		(BField* field1,
												 BField* field2);
	private:
		BString				fTitle;
};


//=====================================================================
// Field and column classes for numeric sizes.

class BSizeField : public BField
{
	public:
							BSizeField			(off_t size);
		void				SetSize				(off_t);
		off_t				Size				();

	private:
		off_t				fSize;
};


//--------------------------------------------------------------------

class BSizeColumn : public BTitledColumn
{
	public:
							BSizeColumn			(const char* title,
												 float width,
												 float minWidth,
												 float maxWidth,
												 alignment align = B_ALIGN_LEFT);
		virtual void		DrawField			(BField* field,
												 BRect rect,
												 BView* parent);
		virtual int			CompareFields		(BField* field1,
												 BField* field2);
};


//=====================================================================
// Field and column classes for integers.

class BIntegerField : public BField
{
	public:
							BIntegerField		(int32 number);
		void				SetValue			(int32);
		int32				Value				();

	private:
		int32				fInteger;
};


//--------------------------------------------------------------------

class BIntegerColumn : public BTitledColumn
{
	public:
							BIntegerColumn		(const char* title,
												 float width,
												 float minWidth,
												 float maxWidth,
												 alignment align = B_ALIGN_LEFT);
		virtual void		DrawField			(BField* field,
												 BRect rect,
												 BView* parent);
		virtual int			CompareFields		(BField* field1,
												 BField* field2);
};


//=====================================================================
// Field and column classes for bitmaps

class BBitmapField : public BField
{
	public:
							BBitmapField		(BBitmap* bitmap);
		const BBitmap*		Bitmap				();
		void				SetBitmap			(BBitmap* bitmap);

	private:
		BBitmap*			fBitmap;
};


//--------------------------------------------------------------------

class BBitmapColumn : public BTitledColumn
{
	public:
							BBitmapColumn		(const char* title,
												 float width,
												 float minWidth,
												 float maxWidth,
												 alignment align = B_ALIGN_LEFT);
		virtual void		DrawField			(BField*field,
												 BRect rect,
												 BView* parent);
		virtual int			CompareFields		(BField* field1, BField* field2);
		virtual	bool		AcceptsField        (const BField* field) const;
};
	

//=====================================================================
// Column to display BIntegerField objects as a graph.

class GraphColumn : public BIntegerColumn
{
	public:
							GraphColumn			(const char* name,
												 float width,
												 float minWidth,
												 float maxWidth,
												 alignment align = B_ALIGN_LEFT);
		virtual void		DrawField			(BField*field,
												 BRect rect,
												 BView* parent);
};

#endif

