/*
 * Copyright 2006-2008, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Stefano Ceccherini (stefano.ceccherini@gmail.com)
 */

#ifndef __MENU_PRIVATE_H
#define __MENU_PRIVATE_H


enum menu_states {
	MENU_STATE_TRACKING = 0,
	MENU_STATE_TRACKING_SUBMENU = 1,
	MENU_STATE_CLOSED = 5
};

class BMenu;
class BWindow;

namespace BPrivate {
	
class MenuPrivate {
public:
	MenuPrivate(BMenu *menu);
	
	menu_layout Layout() const;

	void ItemMarked(BMenuItem *item);
	void CacheFontInfo();
	
	float FontHeight() const;
	float Ascent() const;
	BRect Padding() const;
	void GetItemMargins(float *, float *, float *, float *) const;

	bool IsAltCommandKey() const;
	int State(BMenuItem **item = NULL) const;
	
	void Install(BWindow *window);
	void Uninstall();
	void SetSuper(BMenu *menu);
	void SetSuperItem(BMenuItem *item);
	void InvokeItem(BMenuItem *item, bool now = false);	
	void QuitTracking(bool thisMenuOnly = true);
	
private:
	BMenu *fMenu;	
};

};

extern const char *kEmptyMenuLabel;


// Note: since sqrt is slow, we don't use it and return the square of the distance
#define square(x) ((x) * (x))
static inline float
point_distance(const BPoint &pointA, const BPoint &pointB)
{
	return square(pointA.x - pointB.x) + square(pointA.y - pointB.y);
}

#undef square


#endif // __MENU_PRIVATE_H
