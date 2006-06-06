/*
 * Copyright 2002-2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _SCREENSAVER_H
#define _SCREENSAVER_H


#include <BeBuild.h>
#include <DirectWindow.h>
#include <image.h>
#include <Message.h>

class BView;


class BScreenSaver {
	public:
		BScreenSaver(BMessage* archive, image_id thisImage);
		virtual	~BScreenSaver();

		virtual status_t InitCheck();

		virtual status_t StartSaver(BView* view, bool preview);
		virtual void StopSaver();

		virtual void Draw(BView* view, int32 frame);

		// direct screen access
		virtual void DirectConnected(direct_buffer_info* info);
		virtual void DirectDraw(int32 frame);

		// configuration dialog methods
		virtual void StartConfig(BView* configView);
		virtual void StopConfig();

		// Module should fill this in with metadata
		// example: randomizable = true
		virtual void SupplyInfo(BMessage* info) const;

		// Send all the metadata info to the module
		virtual void ModulesChanged(const BMessage* info);

		// BArchivable like parameter saver method
		virtual	status_t SaveState(BMessage* into) const;

		// These methods can be used to control drawing frequency.
		void SetTickSize(bigtime_t tickSize);
		bigtime_t TickSize() const;

		// These methods can be used to control animation loop cycles
		void SetLoop(int32 onCount, int32 offCount);
		int32 LoopOnCount() const;
		int32 LoopOffCount() const;

	private:
		virtual	void _ReservedScreenSaver1();
		virtual	void _ReservedScreenSaver2();
		virtual	void _ReservedScreenSaver3();
		virtual	void _ReservedScreenSaver4();
		virtual	void _ReservedScreenSaver5();
		virtual	void _ReservedScreenSaver6();
		virtual	void _ReservedScreenSaver7();
		virtual	void _ReservedScreenSaver8();

		bigtime_t	fTickSize;
		int32		fLoopOnCount;
		int32		fLoopOffCount;

		uint32		_reserved[6];
};

extern "C" _EXPORT BScreenSaver *instantiate_screen_saver(BMessage *msg, image_id id);

#endif	// _SCREENSAVER_H
