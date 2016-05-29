/*
 * Copyright 2014, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef IMAGE_DEBUG_LOADING_STATE_HANDLER_ROSTER_H
#define IMAGE_DEBUG_LOADING_STATE_HANDLER_ROSTER_H


#include <Locker.h>
#include <ObjectList.h>


class ImageDebugLoadingStateHandler;
class SpecificImageDebugInfoLoadingState;


typedef BObjectList<ImageDebugLoadingStateHandler> LoadingStateHandlerList;


class ImageDebugLoadingStateHandlerRoster {
public:
								ImageDebugLoadingStateHandlerRoster();
								~ImageDebugLoadingStateHandlerRoster();

	static	ImageDebugLoadingStateHandlerRoster*
								Default();
	static	status_t			CreateDefault();
	static	void				DeleteDefault();

			status_t			Init();
			status_t			RegisterDefaultHandlers();

			status_t			FindStateHandler(
									SpecificImageDebugInfoLoadingState* state,
									ImageDebugLoadingStateHandler*&
										_handler);
									// returns a reference

			bool				RegisterHandler(
									ImageDebugLoadingStateHandler*
										handler);
			void				UnregisterHandler(
									ImageDebugLoadingStateHandler*
										handler);

private:
			BLocker				fLock;
			LoadingStateHandlerList
								fStateHandlers;
	static	ImageDebugLoadingStateHandlerRoster*
								sDefaultInstance;
};


#endif	// IMAGE_DEBUG_LOADING_STATE_HANDLER_ROSTER_H

