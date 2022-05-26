/*
 * Copyright 2005-2009, Axel Dörfler, axeld@pinc-software.de.
 * This file may be used under the terms of the MIT License.
 */


#include "ScreenConfigurations.h"

#include <new>
#include <string.h>
#include <strings.h>

#include <Message.h>


ScreenConfigurations::ScreenConfigurations()
	:
	fConfigurations(10, true)
{
}


ScreenConfigurations::~ScreenConfigurations()
{
}


screen_configuration*
ScreenConfigurations::CurrentByID(int32 id) const
{
	for (int32 i = fConfigurations.CountItems(); i-- > 0;) {
		screen_configuration* configuration = fConfigurations.ItemAt(i);

		if (configuration->id == id && configuration->is_current)
			return configuration;
	}

	return NULL;
}


screen_configuration*
ScreenConfigurations::BestFit(int32 id, const monitor_info* info,
	bool* _exactMatch) const
{
	if (info == NULL) {
		// Only look for a matching ID - this is all we have
		for (uint32 pass = 0; pass < 2; pass++) {
			for (int32 i = fConfigurations.CountItems(); i-- > 0;) {
				screen_configuration* configuration = fConfigurations.ItemAt(i);

				if ((pass != 0 || !configuration->has_info)
					&& id == configuration->id)
					return configuration;
			}
		}

		return NULL;
	}

	// Look for a configuration that matches the monitor

	bool exactMatch = false;
	int32 bestScore = 0;
	int32 bestIndex = -1;
	BMessage stored;

	for (int32 i = fConfigurations.CountItems(); i-- > 0;) {
		screen_configuration* configuration = fConfigurations.ItemAt(i);
		if (!configuration->has_info)
			continue;

		int32 score = 0;

		if (!strcasecmp(configuration->info.vendor, info->vendor)
			&& !strcasecmp(configuration->info.name, info->name)
			&& configuration->info.product_id == info->product_id) {
			score += 2;
			if (strcmp(configuration->info.serial_number,
					info->serial_number) == 0) {
				exactMatch = true;
				score += 2;
			}
			if (configuration->info.produced.year == info->produced.year
				&& configuration->info.produced.week == info->produced.week)
				score++;
		}

		if (score > bestScore) {
			bestScore = score;
			bestIndex = i;
		}
	}

	if (bestIndex < 0)
		return NULL;

	if (_exactMatch != NULL)
		*_exactMatch = exactMatch;

	return fConfigurations.ItemAt(bestIndex);
}


status_t
ScreenConfigurations::Set(int32 id, const monitor_info* info,
	const BRect& frame, const display_mode& mode)
{
	// Find configuration that we can overwrite

	bool exactMatch;
	screen_configuration* configuration = BestFit(id, info, &exactMatch);

	if (configuration != NULL && configuration->has_info && !exactMatch) {
		// only overwrite exact or unspecified configurations
		configuration->is_current = false;
			// TODO: provide a more obvious current mechanism...
		configuration = NULL;
	}

	if (configuration == NULL) {
		// we need a new configuration to store
		configuration = new (std::nothrow) screen_configuration;
		if (configuration == NULL)
			return B_NO_MEMORY;

		fConfigurations.AddItem(configuration);
	}

	configuration->id = id;
	configuration->frame = frame;
	configuration->is_current = true;

	if (info != NULL) {
		memcpy(&configuration->info, info, sizeof(monitor_info));
		configuration->has_info = true;
	} else
		configuration->has_info = false;

	memcpy(&configuration->mode, &mode, sizeof(display_mode));

	return B_OK;
}


void
ScreenConfigurations::SetBrightness(int32 id, float brightness)
{
	for (int32 i = fConfigurations.CountItems(); i-- > 0;) {
		screen_configuration* configuration = fConfigurations.ItemAt(i);
		configuration->brightness = brightness;
	}
}


float
ScreenConfigurations::Brightness(int32 id)
{
	screen_configuration* configuration = fConfigurations.ItemAt(0);

	if (configuration == NULL)
		return -1;

	return configuration->brightness;
}


void
ScreenConfigurations::Remove(screen_configuration* configuration)
{
	if (configuration == NULL)
		return;

	fConfigurations.RemoveItem(configuration);
		// this also deletes the configuration
}


/*!	Stores all configurations as separate BMessages into the provided
	\a settings container.
*/
status_t
ScreenConfigurations::Store(BMessage& settings) const
{
	// Store the configuration of all current screens

	for (int32 i = 0; i < fConfigurations.CountItems(); i++) {
		screen_configuration* configuration = fConfigurations.ItemAt(i);

		BMessage screenSettings;
		screenSettings.AddInt32("id", configuration->id);

		if (configuration->has_info) {
			screenSettings.AddString("vendor", configuration->info.vendor);
			screenSettings.AddString("name", configuration->info.name);
			screenSettings.AddInt32("product id",
				configuration->info.product_id);
			screenSettings.AddString("serial",
				configuration->info.serial_number);
			screenSettings.AddInt32("produced week",
				configuration->info.produced.week);
			screenSettings.AddInt32("produced year",
				configuration->info.produced.year);
		}

		screenSettings.AddRect("frame", configuration->frame);
		screenSettings.AddData("mode", B_RAW_TYPE, &configuration->mode,
			sizeof(display_mode));
		screenSettings.AddFloat("brightness", configuration->brightness);

		settings.AddMessage("screen", &screenSettings);
	}

	return B_OK;
}


status_t
ScreenConfigurations::Restore(const BMessage& settings)
{
	fConfigurations.MakeEmpty();

	BMessage stored;
	for (int32 i = 0; settings.FindMessage("screen", i, &stored) == B_OK; i++) {
		const display_mode* mode;
		ssize_t size;
		int32 id;
		if (stored.FindInt32("id", &id) != B_OK
			|| stored.FindData("mode", B_RAW_TYPE, (const void**)&mode,
					&size) != B_OK
			|| size != sizeof(display_mode))
			continue;

		screen_configuration* configuration
			= new(std::nothrow) screen_configuration;
		if (configuration == NULL)
			return B_NO_MEMORY;

		configuration->id = id;
		configuration->is_current = false;

		const char* vendor;
		const char* name;
		uint32 productID;
		const char* serial;
		int32 week, year;
		if (stored.FindString("vendor", &vendor) == B_OK
			&& stored.FindString("name", &name) == B_OK
			&& stored.FindInt32("product id", (int32*)&productID) == B_OK
			&& stored.FindString("serial", &serial) == B_OK
			&& stored.FindInt32("produced week", &week) == B_OK
			&& stored.FindInt32("produced year", &year) == B_OK) {
			// create monitor info
			strlcpy(configuration->info.vendor, vendor,
				sizeof(configuration->info.vendor));
			strlcpy(configuration->info.name, name,
				sizeof(configuration->info.name));
			strlcpy(configuration->info.serial_number, serial,
				sizeof(configuration->info.serial_number));
			configuration->info.product_id = productID;
			configuration->info.produced.week = week;
			configuration->info.produced.year = year;
			configuration->has_info = true;
		} else
			configuration->has_info = false;

		stored.FindRect("frame", &configuration->frame);
		memcpy(&configuration->mode, mode, sizeof(display_mode));

		if (stored.FindFloat("brightness", &configuration->brightness) != B_OK)
			configuration->brightness = 1.0f;

		fConfigurations.AddItem(configuration);
	}

	return B_OK;
}
