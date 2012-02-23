/*
 * Copyright 2008, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef ACTIVITY_VIEW_H
#define ACTIVITY_VIEW_H


#include <map>

#include <Locker.h>
#include <Message.h>
#include <ObjectList.h>
#include <View.h>

#include "CircularBuffer.h"
#include "DataSource.h"

class BBitmap;
class BMessageRunner;
class Scale;
class SystemInfoHandler;
class ViewHistory;
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
			void		SetScale(Scale* scale);

private:
	CircularBuffer<data_item> fBuffer;
	int64				fMinimumValue;
	int64				fMaximumValue;
	bigtime_t			fRefreshInterval;
	int32				fLastIndex;
	Scale*				fScale;
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

#ifdef __HAIKU__
			BLayoutItem* CreateHistoryLayoutItem();
			BLayoutItem* CreateLegendLayoutItem();
#endif

			DataSource*	FindDataSource(const DataSource* source);
			status_t	AddDataSource(const DataSource* source,
							const BMessage* state = NULL);
			status_t	RemoveDataSource(const DataSource* source);
			void		RemoveAllDataSources();

			bigtime_t	RefreshInterval() const
							{ return atomic_get64((vint64*)&fRefreshInterval); }

protected:
	virtual	void		AttachedToWindow();
	virtual	void		DetachedFromWindow();

#ifdef __HAIKU__
	virtual	BSize		MinSize();
#endif

	virtual void		FrameResized(float width, float height);
	virtual void		MouseDown(BPoint where);
	virtual void		MouseUp(BPoint where);
	virtual void		MouseMoved(BPoint where, uint32 transit,
							const BMessage* dragMessage);

	virtual void		MessageReceived(BMessage* message);

	virtual void		Draw(BRect updateRect);

private:
			void		_Init(const BMessage* settings);
			::Scale*	_ScaleFor(scale_type type);
			void		_Refresh();
	static	status_t	_RefreshThread(void* self);
			void		_UpdateOffscreenBitmap();
			BView*		_OffscreenView();
			void		_UpdateFrame();
			BRect		_HistoryFrame() const;
			float		_LegendHeight() const;
			BRect		_LegendFrame() const;
			BRect		_LegendFrameAt(BRect frame, int32 index) const;
			BRect		_LegendColorFrameAt(BRect frame, int32 index) const;
			float		_PositionForValue(DataSource* source,
							DataHistory* values, int64 value);
			void		_DrawHistory(bool drawBackground);
			void		_UpdateResolution(int32 resolution,
							bool broadcast = true);

private:
	class HistoryLayoutItem;
	class LegendLayoutItem;

	friend class HistoryLayoutItem;
	friend class LegendLayoutItem;

	rgb_color			fHistoryBackgroundColor;
	rgb_color			fLegendBackgroundColor;
	BBitmap*			fOffscreen;
#ifdef __HAIKU__
	BLayoutItem*		fHistoryLayoutItem;
	BLayoutItem*		fLegendLayoutItem;
#endif

	mutable BLocker		fSourcesLock;
	BObjectList<DataSource> fSources;
	BObjectList<DataHistory> fValues;
	BObjectList<ViewHistory> fViewValues;
	thread_id			fRefreshThread;
	sem_id				fRefreshSem;
	bigtime_t			fRefreshInterval;
	bigtime_t			fLastRefresh;
	int32				fDrawResolution;
	bool				fShowLegend;
	bool				fZooming;
	BPoint				fZoomPoint;
	int32				fOriginalResolution;
	SystemInfoHandler*	fSystemInfoHandler;
	std::map<scale_type, ::Scale*> fScales;
};

#endif	// ACTIVITY_VIEW_H
