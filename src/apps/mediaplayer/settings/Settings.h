/*
 * Copyright 2008-2011, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Fredrik Modéen <fredrik@modeen.se>
 */
#ifndef SETTINGS_H
#define SETTINGS_H


#include <Entry.h>
#include <Locker.h>

#include "Notifier.h"
#include "SettingsMessage.h"


#define SETTINGS_FILENAME "MediaPlayer"


struct mpSettings {
			enum {
				SUBTITLE_SIZE_SMALL = 0,
				SUBTITLE_SIZE_MEDIUM = 1,
				SUBTITLE_SIZE_LARGE = 2
			};
			enum {
				SUBTITLE_PLACEMENT_BOTTOM_OF_VIDEO = 0,
				SUBTITLE_PLACEMENT_BOTTOM_OF_SCREEN = 1
			};
			enum {
				BG_MOVIES_FULL_VOLUME = 0,
				BG_MOVIES_HALF_VLUME = 1,
				BG_MOVIES_MUTED = 2
			};

			bool				autostart;
			bool				closeWhenDonePlayingMovie;
			bool				closeWhenDonePlayingSound;
			bool				loopMovie;
			bool				loopSound;
			bool				useOverlays;
			bool				scaleBilinear;
			bool				scaleFullscreenControls;
			uint32				subtitleSize;
			uint32				subtitlePlacement;
			uint32				backgroundMovieVolumeMode;
			entry_ref			filePanelFolder;

			bool				operator!=(const mpSettings& other) const;

			BRect				audioPlayerWindowFrame;
};


class Settings : public BLocker, public Notifier {
public:
								Settings(
									const char* filename = SETTINGS_FILENAME);

			void				Get(mpSettings& settings) const;
			void				Update(const mpSettings& settings);

			entry_ref			FilePanelFolder() const;
			void				SetFilePanelFolder(const entry_ref& ref);

			BRect				AudioPlayerWindowFrame() const;
			void				SetAudioPlayerWindowFrame(BRect frame);

	static	Settings*			Default();

private:
			SettingsMessage		fSettingsMessage;
			BList				fListeners;

	static	Settings			sGlobalInstance;
};


#endif  // SETTINGS_H
