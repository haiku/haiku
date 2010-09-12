/*
 * Copyright 2010, Stephan Aßmus <superstippi@gmx.de>.
 * Distributed under the terms of the MIT License.
 */
#ifndef DURATION_VIEW_H
#define DURATION_VIEW_H


#include <String.h>
#include <StringView.h>


class DurationView : public BStringView {
public:
								DurationView(const char* name);

	// BStringView interface
	virtual	void				AttachedToWindow();
	virtual	void				MouseDown(BPoint where);
	virtual	BSize				MinSize();
	virtual	BSize				MaxSize();

	// DurationView
			void				Update(bigtime_t position, bigtime_t duration);

			enum {
				kTimeElapsed = 0,
				kTimeToFinish,
				kDuration,

				kLastMode
			};

			void				SetMode(uint32 mode);
			uint32				Mode() const
									{ return fMode; }

			void				SetSymbolScale(float scale);

private:
			void				_Update();
			void				_GenerateString(bigtime_t duration);

private:
			uint32				fMode;
			bigtime_t			fPosition;
			bigtime_t			fDuration;
			bigtime_t			fDisplayDuration;
};


#endif	// DURATION_VIEW_H
