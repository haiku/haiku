/*
 * Copyright 2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef POSITION_TOOL_TIP_H
#define POSITION_TOOL_TIP_H


#include <ToolTip.h>


class PositionToolTip : public BToolTip {
public:
								PositionToolTip();
	virtual						~PositionToolTip();

	virtual BView*				View() const;

	void						Update(bigtime_t position, bigtime_t duration);

private:
	class PositionView;

	PositionView*				fView;
};

#endif	// POSITION_TOOL_TIP_H
