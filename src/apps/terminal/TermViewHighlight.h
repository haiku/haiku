/*
 * Copyright 2013, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef TERMVIEW_HIGHLIGHT_H
#define TERMVIEW_HIGHLIGHT_H


#include <GraphicsDefs.h>

#include "TermPos.h"


class TermViewHighlighter {
public:
	virtual						~TermViewHighlighter();

	virtual	rgb_color			ForegroundColor() = 0;
	virtual	rgb_color			BackgroundColor() = 0;
	virtual	uint32				AdjustTextAttributes(uint32 attributes);
};


class TermViewHighlight {
public:
	TermViewHighlight()
		:
		fHighlighter(NULL),
		fStart(-1, -1),
		fEnd(-1, -1)
	{
	}

	TermViewHighlighter* Highlighter() const
	{
		return fHighlighter;
	}

	void SetHighlighter(TermViewHighlighter* highligher)
	{
		fHighlighter = highligher;
	}

	const TermPos& Start() const
	{
		return fStart;
	}

	const TermPos& End() const
	{
		return fEnd;
	}

	bool IsEmpty() const
	{
		return fStart == fEnd;
	}

	bool RangeContains(const TermPos& pos) const
	{
		return pos >= fStart && pos < fEnd;
	}

	void SetRange(const TermPos& start, const TermPos& end)
	{
		fStart = start;
		fEnd = end;
	}

	void ScrollRange(int32 byLines)
	{
		fStart.y -= byLines;
		fEnd.y -= byLines;
	}

private:
	TermViewHighlighter*	fHighlighter;
	TermPos					fStart;
	TermPos					fEnd;
};


#endif // TERMVIEW_HIGHLIGHT_H
