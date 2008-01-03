#ifndef __MENU_PRIVATE_H
#define __MENU_PRIVATE_H


enum menu_states {
	MENU_STATE_TRACKING = 0,
	MENU_STATE_TRACKING_SUBMENU = 1,
	MENU_STATE_CLOSED = 5
};

extern const char *kEmptyMenuLabel;


// Note: since sqrt is slow, we don't use it and return the square of the distance
#define square(x) ((x) * (x))
static inline float
point_distance(const BPoint &pointA, const BPoint &pointB)
{
	return square(pointA.x - pointB.x) + square(pointA.y - pointB.y);
}

/*
static float
point_rect_distance(const BPoint &point, const BRect &rect)
{
	float horizontal = 0;
	float vertical = 0;
	if (point.x < rect.left)
		horizontal = rect.left - point.x;
	else if (point.x > rect.right)
		horizontal = point.x - rect.right;

	if (point.y < rect.top)
		vertical = rect.top - point.y;
	else if (point.y > rect.bottom)
		vertical = point.y - rect.bottom;
	
	return square(horizontal) + square(vertical);
}
*/

#undef square


#endif // __MENU_PRIVATE_H
