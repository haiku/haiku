/*
Open Tracker License

Terms and Conditions

Copyright (c) 1991-2000, Be Incorporated. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice applies to all licensees
and shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF TITLE, MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
BE INCORPORATED BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF, OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of Be Incorporated shall not be
used in advertising or otherwise to promote the sale, use or other dealings in
this Software without prior written authorization from Be Incorporated.

Tracker(TM), Be(R), BeOS(R), and BeIA(TM) are trademarks or registered trademarks
of Be Incorporated in the United States and other countries. Other brand product
names are registered trademarks or trademarks of their respective holders.
All rights reserved.
*/
#ifndef __TEXT_WIDGET_ATTRIBUTE__
#define __TEXT_WIDGET_ATTRIBUTE__


#include <String.h>

#include "TrackerSettings.h"


namespace BPrivate {

class Model;
class BPoseView;
class BColumn;

// Tracker-only type for truncating the size string
// (Used in InfoWindow.cpp)
const uint32 kSizeType = 'kszt';

class WidgetAttributeText {
	// each of subclasses knows how to retrieve a specific attribute
	// from a model that is passed in and knows how to display the
	// corresponding text, fitted into a specified width using a given
	// view
	// It is being asked for the string value by the TextWidget object
	public:
		WidgetAttributeText(const Model*, const BColumn*);
		virtual ~WidgetAttributeText();

		virtual bool CheckAttributeChanged() = 0;
			// returns true if attribute value changed

		bool CheckViewChanged(const BPoseView*);
			// returns true if fitted text changed, either because value
			// changed or because width/view changed

		virtual bool CheckSettingsChanged();
			// override if the text rendering depends on a setting

		const char* FittingText(const BPoseView*);
			// returns text, recalculating if not yet calculated

		virtual int Compare(WidgetAttributeText&, BPoseView* view) = 0;
			// override to define a compare of two different attributes for
			// sorting

		static WidgetAttributeText* NewWidgetText(const Model*,
			const BColumn*, const BPoseView*);
			// WidgetAttributeText factory
			// call this to make the right WidgetAttributeText type for a
			// given column

		float Width(const BPoseView*);
			// respects the width of the corresponding column
		float CurrentWidth() const;
			// return the item width we got during our last fitting attempt

		virtual void SetUpEditing(BTextView*);
			// set up the passed textView for the specifics of a given
			// attribute editing
		virtual bool CommitEditedText(BTextView*) = 0;
			// return true if attribute actually changed

		virtual float PreferredWidth(const BPoseView*) const = 0;

		static status_t AttrAsString(const Model* model, BString* result,
			const char* attrName, int32 attrType, float width,
			BView* view, int64* value = 0);

		Model* TargetModel() const;

		virtual bool IsEditable() const;

		void SetDirty(bool);

	protected:
		// generic fitting routines used by the different attributes
		static float TruncString(BString* result, const char* src,
			int32 length, const BPoseView*, float width,
			uint32 truncMode = B_TRUNCATE_MIDDLE);

		static float TruncTime(BString* result, int64 src,
			const BPoseView* view, float width);

		static float TruncFileSize(BString* result, int64 src,
			const BPoseView* view, float width);

		virtual void FitValue(BString* result, const BPoseView*) = 0;
			// override FitValue to do a specific text fitting for a given
			// attribute

		mutable Model* fModel;
		const BColumn* fColumn;
		// TODO: make these int32 only
		float fOldWidth;
		float fTruncatedWidth;
		bool fDirty;
			// if true, need to recalculate text next time we try to use it
	 	bool fValueIsDefined;
	 	BString fText;
			// holds the truncated text, fit to the parameters passed in
			// in the last FittingText call
};

inline Model*
WidgetAttributeText::TargetModel() const
{
	return fModel;
}


class StringAttributeText : public WidgetAttributeText {
	public:
		StringAttributeText(const Model*, const BColumn*);

		virtual const char* ValueAsText(const BPoseView* view);
			// returns the untrucated text that corresponds to
			// the attribute value

		virtual bool CheckAttributeChanged();

		virtual float PreferredWidth(const BPoseView*) const;

		virtual bool CommitEditedText(BTextView*);

	protected:
		virtual bool CommitEditedTextFlavor(BTextView*) { return false; }

		virtual void FitValue(BString* result, const BPoseView*);
		virtual void ReadValue(BString* result) = 0;

		virtual int Compare(WidgetAttributeText &, BPoseView* view);

		BString fFullValueText;
		bool fValueDirty;
			// used for lazy read, managed by ReadValue
};


class ScalarAttributeText : public WidgetAttributeText {
	public:
		ScalarAttributeText(const Model*, const BColumn*);
		int64 Value();
		virtual bool CheckAttributeChanged();

		virtual float PreferredWidth(const BPoseView*) const;

		virtual bool CommitEditedText(BTextView*) { return false; }
			// return true if attribute actually changed
	protected:
		virtual int64 ReadValue() = 0;
		virtual int Compare(WidgetAttributeText&, BPoseView* view);
		int64 fValue;
		bool fValueDirty;
			// used for lazy read, managed by ReadValue
};


union GenericValueStruct {
	time_t	time_tt;
	off_t	off_tt;

	bool	boolt;
	int8	int8t;
	uint8	uint8t;
	int16	int16t;
	int16	uint16t;
	int32	int32t;
	int32	uint32t;
	int64	int64t;
	int64	uint64t;

	float	floatt;
	double	doublet;
};


//! Used for displaying mime extra attributes. Supports different formats.
class GenericAttributeText : public StringAttributeText {
public:
								GenericAttributeText(const Model* model,
									const BColumn* column);

	virtual	bool				CheckAttributeChanged();
	virtual	float				PreferredWidth(const BPoseView* view) const;

	virtual	int					Compare(WidgetAttributeText& other,
									BPoseView* view);

	virtual	void				SetUpEditing(BTextView* view);
	virtual	bool				CommitEditedText(BTextView* view);

	virtual	const char*			ValueAsText(const BPoseView* view);

protected:
	virtual	bool				CommitEditedTextFlavor(BTextView* view);

	virtual	void				FitValue(BString* result,
									const BPoseView* view);
	virtual void				ReadValue(BString* result);

protected:
	// TODO: split this up into a scalar flavor and string flavor
	// to save memory
			GenericValueStruct	fValue;
};


//! Used for the display-as type "duration"
class DurationAttributeText : public GenericAttributeText {
public:
								DurationAttributeText(const Model* model,
									const BColumn* column);

private:
	virtual	void				FitValue(BString* result,
									const BPoseView* view);
};


//! Used for the display-as type "checkbox"
class CheckboxAttributeText : public GenericAttributeText {
public:
								CheckboxAttributeText(const Model* model,
									const BColumn* column);

	virtual	void				SetUpEditing(BTextView* view);

private:
	virtual	void				FitValue(BString* result,
									const BPoseView* view);

private:
			BString				fOnChar;
			BString				fOffChar;
};


//! Used for the display-as type "rating"
class RatingAttributeText : public GenericAttributeText {
public:
								RatingAttributeText(const Model* model,
									const BColumn* column);

	virtual	void				SetUpEditing(BTextView* view);

private:
	virtual	void				FitValue(BString* result,
									const BPoseView* view);

private:
			int32				fCount;
			int32				fMax;
};


class TimeAttributeText : public ScalarAttributeText {
	public:
		TimeAttributeText(const Model*, const BColumn*);
	protected:
		virtual float PreferredWidth(const BPoseView*) const;
		virtual void FitValue(BString* result, const BPoseView*);
		virtual bool CheckSettingsChanged();

		TrackerSettings fSettings;
		bool fLastClockIs24;
		DateOrder fLastDateOrder;
		FormatSeparator fLastTimeFormatSeparator;
};


class PathAttributeText : public StringAttributeText {
	public:
		PathAttributeText(const Model*, const BColumn*);
	protected:
		virtual void ReadValue(BString* result);
};


class OriginalPathAttributeText : public StringAttributeText {
	public:
		OriginalPathAttributeText(const Model*, const BColumn*);
	protected:
		virtual void ReadValue(BString* result);
};


class KindAttributeText : public StringAttributeText {
	public:
		KindAttributeText(const Model*, const BColumn*);
	protected:
		virtual void ReadValue(BString* result);
};


class NameAttributeText : public StringAttributeText {
	public:
		NameAttributeText(const Model*, const BColumn*);
		virtual void SetUpEditing(BTextView*);
		virtual void FitValue(BString* result, const BPoseView*);
		virtual bool IsEditable() const;

		static void SetSortFolderNamesFirst(bool);
	protected:
		virtual bool CommitEditedTextFlavor(BTextView*);
		virtual int Compare(WidgetAttributeText&, BPoseView* view);
		virtual void ReadValue(BString* result);

		static bool sSortFolderNamesFirst;
};


class RealNameAttributeText : public StringAttributeText {
public:
							RealNameAttributeText(const Model*,
								const BColumn*);
	virtual	void			SetUpEditing(BTextView*);
	virtual	void			FitValue(BString* result, const BPoseView*);

	static	void			SetSortFolderNamesFirst(bool);

protected:
	virtual	bool			CommitEditedTextFlavor(BTextView*);
	virtual	int				Compare(WidgetAttributeText&, BPoseView* view);
	virtual	void			ReadValue(BString* result);

	static	bool			sSortFolderNamesFirst;
};


#ifdef OWNER_GROUP_ATTRIBUTES

class OwnerAttributeText : public StringAttributeText {
	public:
		OwnerAttributeText(const Model*, const BColumn*);

	protected:
		virtual void ReadValue(BString* result);
};


class GroupAttributeText : public StringAttributeText {
	public:
		GroupAttributeText(const Model*, const BColumn*);

	protected:
		virtual void ReadValue(BString* result);
};

#endif  // OWNER_GROUP_ATTRIBUTES


class ModeAttributeText : public StringAttributeText {
	public:
		ModeAttributeText(const Model*, const BColumn*);

	protected:
		virtual void ReadValue(BString* result);
};


const int64 kUnknownSize = -1;


class SizeAttributeText : public ScalarAttributeText {
	public:
		SizeAttributeText(const Model*, const BColumn*);

	protected:
		virtual void FitValue(BString* result, const BPoseView*);
		virtual int64 ReadValue();
		virtual float PreferredWidth(const BPoseView*) const;
};


class CreationTimeAttributeText : public TimeAttributeText {
	public:
		CreationTimeAttributeText(const Model*, const BColumn*);
	protected:
		virtual int64 ReadValue();
};


class ModificationTimeAttributeText : public TimeAttributeText {
	public:
		ModificationTimeAttributeText(const Model*, const BColumn*);

	protected:
		virtual int64 ReadValue();
};


class OpenWithRelationAttributeText : public ScalarAttributeText {
	public:
		OpenWithRelationAttributeText(const Model*, const BColumn*,
			const BPoseView*);

	protected:
		virtual void FitValue(BString* result, const BPoseView*);
		virtual int64 ReadValue();
		virtual float PreferredWidth(const BPoseView*) const;

		const BPoseView* fPoseView;
		BString fRelationText;
};


class VersionAttributeText : public StringAttributeText {
	public:
		VersionAttributeText(const Model*, const BColumn*, bool appVersion);

	protected:
		virtual void ReadValue(BString* result);

	private:
		bool fAppVersion;
};


class AppShortVersionAttributeText : public VersionAttributeText {
	public:
		AppShortVersionAttributeText(const Model* model,
			const BColumn* column)
			:	VersionAttributeText(model, column, true)
		{
		}
};


class SystemShortVersionAttributeText : public VersionAttributeText {
	public:
		SystemShortVersionAttributeText(const Model* model,
			const BColumn* column)
			:	VersionAttributeText(model, column, false)
		{
		}
};

} // namespace BPrivate


extern status_t TimeFormat(BString &string, int32 index,
	FormatSeparator format, DateOrder order, bool clockIs24Hour);

using namespace BPrivate;

#endif	// __TEXT_WIDGET_ATTRIBUTE__
