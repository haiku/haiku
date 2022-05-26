/*
 * Copyright 2009, Axel Dörfler, axeld@pinc-software.de.
 * This file may be used under the terms of the MIT License.
 */
#ifndef SCREEN_CONFIGURATIONS_H
#define SCREEN_CONFIGURATIONS_H


#include <Accelerant.h>
#include <Rect.h>

#include <ObjectList.h>


class BMessage;


struct screen_configuration {
	int32			id;
	monitor_info	info;
	BRect			frame;
	display_mode	mode;
	float			brightness;
	bool			has_info;
	bool			is_current;
};


class ScreenConfigurations {
public:
								ScreenConfigurations();
								~ScreenConfigurations();

			screen_configuration* CurrentByID(int32 id) const;
			screen_configuration* BestFit(int32 id, const monitor_info* info,
									bool* _exactMatch = NULL) const;

			status_t			Set(int32 id, const monitor_info* info,
									const BRect& frame,
									const display_mode& mode);
			void				SetBrightness(int32 id, float brightness);
			float				Brightness(int32 id);
			void				Remove(screen_configuration* configuration);

			status_t			Store(BMessage& settings) const;
			status_t			Restore(const BMessage& settings);

private:
	typedef BObjectList<screen_configuration> ConfigurationList;

			ConfigurationList	fConfigurations;
};


#endif	// SCREEN_CONFIGURATIONS_H

