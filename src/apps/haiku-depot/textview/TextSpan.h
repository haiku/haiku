/*
 * Copyright 2013, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef TEXT_SPAN_H
#define TEXT_SPAN_H


#include <String.h>

#include "TextStyle.h"


class TextSpan {
public:
								TextSpan();
								TextSpan(const BString& text,
									const TextStyle& style);
								TextSpan(const TextSpan& other);

			TextSpan&			operator=(const TextSpan& other);
			bool				operator==(const TextSpan& other) const;
			bool				operator!=(const TextSpan& other) const;

			void				SetText(const BString& text);
	inline	const BString&		Text() const
									{ return fText; }

			void				SetStyle(const TextStyle& style);
	inline	const TextStyle&	Style() const
									{ return fStyle; }

			TextSpan			SubSpan(int32 start, int32 count) const;

private:
			BString				fText;
			TextStyle			fStyle;
};


#endif // TEXT_SPAN_H
