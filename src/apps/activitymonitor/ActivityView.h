/*
 * Copyright 2008, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef ACTIVITY_VIEW_H
#define ACTIVITY_VIEW_H


#include <View.h>
#include <ObjectList.h>

#include "CircularBuffer.h"

class BBitmap;
class BMessageRunner;
class DataSource;
struct data_item;


class DataHistory {
public:
						DataHistory(bigtime_t memorize, bigtime_t interval);
						~DataHistory();

			void		AddValue(bigtime_t time, int64 value);

			int64		ValueAt(bigtime_t time);
			int64		MaximumValue() const;
			int64		MinimumValue() const;
			bigtime_t	Start() const;
			bigtime_t	End() const;

			void		SetRefreshInterval(bigtime_t interval);

private:
	CircularBuffer<data_item> fBuffer;
	int64				fMinimumValue;
	int64				fMaximumValue;
	bigtime_t			fRefreshInterval;
	int32				fLastIndex;
};


class ActivityView : public BView {
public:
						ActivityView(BRect frame, const char* name,
							const BMessage* settings, uint32 resizingMode);
						ActivityView(const char* name,
							const BMessage* settings);
						ActivityView(BMessage* archive);
	virtual				~ActivityView();

	virtual status_t	Archive(BMessage* into, bool deep = true) const;
	static	BArchivable* Instantiate(BMessage* archive);

			status_t	SaveState(BMessage& state) const;

			DataSource*	FindDataSource(const DataSource* source);
			status_t	AddDataSource(const DataSource* source);
			status_t	RemoveDataSource(const DataSource* source);
			void		RemoveAllDataSources();

protected:
	virtual	void		AttachedToWindow();
	virtual	void		DetachedFromWindow();

#ifdef __HAIKU__
	virtual	BSize		MinSize();
#endif

	virtual void		FrameResized(float width, float height);
	virtual void		MouseDown(BPoint where);
	virtual void		MouseMoved(BPoint where, uint32 transit,
							const BMessage* dragMessage);

	virtual void		MessageReceived(BMessage* message);

	virtual void		Draw(BRect updateRect);

private:
			void		_Init(const BMessage* settings);
			void		_Refresh();
			void		_UpdateOffscreenBitmap();
			BRect		_HistoryFrame() const;
			BRect		_LegendFrame() const;
			BRect		_LegendFrameAt(BRect frame, int32 index) const;
			float		_PositionForValue(DataSource* source,
							DataHistory* values, int64 value);
			void		_DrawHistory();

	rgb_color			fBackgroundColor;
	BBitmap*			fOffscreen;
	BObjectList<DataSource> fSources;
	BObjectList<DataHistory> fValues;
	BMessageRunner*		fRunner;
	bigtime_t			fRefreshInterval;
	bigtime_t			fLastRefresh;
	bigtime_t			fDrawInterval;
	int32				fDrawResolution;
	bool				fShowLegend;
};

#endif	// ACTIVITY_VIEW_H
