/*
 * Copyright 2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef SETTINGS_H
#define SETTINGS_H


#include <Point.h>
#include <Volume.h>


class Settings {
	public :
		Settings();
		virtual ~Settings();

		BPoint WindowPosition() const { return fWindowPosition; }
		void SetWindowPosition(BPoint position);

		bool SwapEnabled() const { return fSwapEnabled; }
		off_t SwapSize() const { return fSwapSize; }
		BVolume& SwapVolume() { return fSwapVolume; }
		void SetSwapEnabled(bool enabled);
		void SetSwapSize(off_t size);
		void SetSwapVolume(BVolume& volume);

		void SetSwapDefaults();
		void RevertSwapChanges();
		bool SwapChanged();

	private:
		void ReadWindowSettings();
		void WriteWindowSettings();

		void ReadSwapSettings();
		void WriteSwapSettings();

		BPoint		fWindowPosition;

		bool		fSwapEnabled;
		off_t		fSwapSize;
		BVolume		fSwapVolume;

		bool		fInitialSwapEnabled;
		off_t		fInitialSwapSize;
		dev_t		fInitialSwapVolume;

		bool		fPositionUpdated, fSwapUpdated;
};

#endif	/* SETTINGS_H */
