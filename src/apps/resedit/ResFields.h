/*
 * Copyright (c) 2005-2010, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *		DarkWyrm <darkwyrm@gmail.com>
 */
#ifndef RESFIELDS_H
#define RESFIELDS_H

#include <ColumnTypes.h>
#include <Resources.h>

class ResourceData;

class TypeCodeField : public BStringField
{
public:
					TypeCodeField(const type_code &code, ResourceData *data);
	type_code 		GetTypeCode(void) const { return fTypeCode; }
	ResourceData *	GetResourceData(void) const { return fData; }

private:
	type_code		fTypeCode;
	ResourceData	*fData;
};

// This is the base class for fields displaying the preview in the Data
// column of the main window. Each child class must implement all methods
class PreviewField : public BField
{
public:
					PreviewField(void);
	virtual			~PreviewField(void);
	virtual	void	DrawField(BRect rect, BView* parent) = 0;
	virtual void	SetData(char *data, const size_t &length) = 0;
};

// Unlike the BBitmapField class, this one actually takes ownership of the
// bitmap passed to it. This is good because the bitmap given to it is
// allocated by the Translation Kit.
class BitmapPreviewField : public PreviewField
{
public:
					BitmapPreviewField(BBitmap *bitmap);
	virtual			~BitmapPreviewField(void);
	virtual	void	DrawField(BRect rect, BView* parent);
	virtual void	SetData(char *data, const size_t &length);

private:
	BBitmap			*fBitmap;
};

class IntegerPreviewField : public PreviewField
{
public:
					IntegerPreviewField(const int64 &value);
	virtual			~IntegerPreviewField(void);
	virtual	void	DrawField(BRect rect, BView* parent);
	virtual void	SetData(char *data, const size_t &length);

private:
	int64			fValue;
};


class StringPreviewField : public PreviewField
{
public:
					StringPreviewField(const char *string);
	virtual			~StringPreviewField(void);
	virtual	void	DrawField(BRect rect, BView* parent);
	virtual void	SetData(char *data, const size_t &length);

private:
	BString			fString;
	BString			fClipped;
};

BString MakeTypeString(int32 type);

#endif
