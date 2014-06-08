/*
 * Copyright 2014, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef IMAGE_DEBUG_INFO_LOADING_STATE_H
#define IMAGE_DEBUG_INFO_LOADING_STATE_H

#include <Referenceable.h>


class SpecificImageDebugInfoLoadingState;


class ImageDebugInfoLoadingState {
public:
								ImageDebugInfoLoadingState();
	virtual						~ImageDebugInfoLoadingState();

			bool				HasSpecificDebugInfoLoadingState() const;
			SpecificImageDebugInfoLoadingState*
								GetSpecificDebugInfoLoadingState() const
									{ return fSpecificInfoLoadingState; }
			void				SetSpecificDebugInfoLoadingState(
									SpecificImageDebugInfoLoadingState* state);
									// note: takes over reference of passed
									// in state object.
			void				ClearSpecificDebugInfoLoadingState();

			bool				UserInputRequired() const;


			int32				GetSpecificInfoIndex() const
									{ return fSpecificInfoIndex; }
			void				SetSpecificInfoIndex(int32 index);

private:
			BReference<SpecificImageDebugInfoLoadingState>
								fSpecificInfoLoadingState;
			int32				fSpecificInfoIndex;
};


#endif // IMAGE_DEBUG_INFO_LOADING_STATE_H
