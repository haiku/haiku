/*
 * Copyright 2010, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Clemens Zeidler <haiku@clemens-zeidler.de>
 */
#ifndef ANALYSER_DISPATCHER
#define ANALYSER_DISPATCHER


#include <Looper.h>
#include <String.h>

#include "IndexServerAddOn.h"


class FileAnalyser;


class AnalyserDispatcher : public BLooper {
public:
								AnalyserDispatcher(const char* name);
								~AnalyserDispatcher();

			void				Stop();
			bool				Stopped();

			bool				Busy();

			void				AnalyseEntry(const entry_ref& ref);
			void				DeleteEntry(const entry_ref& ref);
			void				MoveEntry(const entry_ref& oldRef,
									const entry_ref& newRef);
			void				LastEntry();

			//! thread safe
			bool				AddAnalyser(FileAnalyser* analyser);
			bool				RemoveAnalyser(const BString& name);

			void				WriteAnalyserSettings();
			void				SetSyncPosition(bigtime_t time);
			void				SetWatchingStart(bigtime_t time);
			void				SetWatchingPosition(bigtime_t time);

protected:
			FileAnalyserList	fFileAnalyserList;

private:
			FileAnalyser*		_FindAnalyser(const BString& name);

			vint32				fStopped;
};

#endif // ANALYSER_DISPATCHER
