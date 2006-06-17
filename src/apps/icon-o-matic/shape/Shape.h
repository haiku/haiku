#ifndef SHAPE_H
#define SHAPE_H

#include <Rect.h>

#include "Observable.h"

class Style;
class VectorPath;

class Shape : public Observable {
 public:
								Shape(VectorPath* path,
									  ::Style* style);
	virtual						~Shape();

			void				SetPath(VectorPath* path);
	inline	VectorPath*			Path() const
									{ return fPath; }

			void				SetStyle(::Style* style);
	inline	::Style*			Style() const
									{ return fStyle; }

			BRect				Bounds() const;

 private:
			VectorPath*			fPath;
			::Style*			fStyle;
};

#endif // SHAPE_H
