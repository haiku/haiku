/*
 * Copyright 2025, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Nathan Patrizi, nathan.patrizi@gmail.com
 */
#ifndef LOCATION_STRING_VIEW_H
#define LOCATION_STRING_VIEW_H


#include <Path.h>
#include <StringView.h>


class LocationStringView : public BStringView {
public:
								LocationStringView(const char* name, const char* text);

			void				MouseDown(BPoint point);
			void				MouseMoved(BPoint point, uint32 transit,
									const BMessage* dragMessage);
			void				CheckAndSetStyleForLink();

private:
			bool				_IsFileLink();
			void				_StripFileProtocol();
			void				_StyleAsLink(bool set);
			void				_MouseAway();
			void				_MouseOver();

private:
			BPath				fFilePath;
			BPath				fFilePathParent;
			rgb_color			fOriginalHighColor;
			bool				fStyledAsLink;
};


#endif	// LOCATION_STRING_VIEW_H
