// Copyright 1999, Be Incorporated. All Rights Reserved.
// Copyright 2000-2004, Jun Suzuki. All Rights Reserved.
// Copyright 2007, Stephan Aßmus. All Rights Reserved.
// This file may be used under the terms of the Be Sample Code License.
#ifndef MEDIA_CONVERTER_WINDOW_H
#define MEDIA_CONVERTER_WINDOW_H


#include <Directory.h>
#include <MediaDefs.h>
#include <MediaFormats.h>
#include <Window.h>


class BBox;
class BButton;
class BFilePanel;
class BMediaFile;
class BMenuField;
class BSlider;
class BStringView;
class BTextControl;
class MediaFileInfoView;
class MediaFileListView;
class StatusView;

class MediaConverterWindow : public BWindow {
public:
								MediaConverterWindow(BRect frame);
	virtual						~MediaConverterWindow();

protected:
	virtual void				MessageReceived(BMessage* message);
	virtual bool				QuitRequested();

public:
			void				LanguageChanged();

			void				BuildFormatMenu();
			void				BuildAudioVideoMenus();
			void				GetSelectedFormatInfo(
									media_file_format** _format,
									media_codec_info** _audio,
									media_codec_info** _video);

			void				SetStatusMessage(const char *message);
			void				SetFileMessage(const char *message);

			bool				AddSourceFile(BMediaFile* file,
									const entry_ref& ref);
			void				RemoveSourceFile(int32 index);
			int32				CountSourceFiles();
			status_t			GetSourceFileAt(int32 index, BMediaFile** _file,
									entry_ref* ref);

			void				SourceFileSelectionChanged();

			void				SetEnabled(bool enabled, bool convertEnabled);
			bool				IsEnabled();

			const char*			StartDuration() const;
			const char*			EndDuration() const;

			int32				AudioQuality() const
									{ return fAudioQuality; }
			int32				VideoQuality() const
									{ return fVideoQuality; }

			void				SetAudioQualityLabel(const char* label);
			void				SetVideoQualityLabel(const char* label);

			BDirectory			OutputDirectory() const;

private:
			void				_UpdateLabels();
			void				_UpdateBBoxLayoutInsets(BBox* box);
			void				_CreateMenu();
			void				_DestroyMenu();
			void				_SetOutputFolder(BEntry entry);

private:
			BButton*			fConvertButton;

			BButton*			fDestButton;
			BButton*			fPreviewButton;
			BBox*				fSourcesBox;
			BBox*				fInfoBox;
			BBox*				fOutputBox;

			BMenuBar*			fMenuBar;
			BMenuField*			fFormatMenu;
			BMenuField*			fVideoMenu;
			BMenuField*			fAudioMenu;
			BStringView*		fFileStatus;
			BStringView*		fStatus;
			MediaFileListView*	fListView;
			MediaFileInfoView*	fInfoView;
			BStringView*		fOutputFolder;
			BTextControl*		fStartDurationTC;
			BTextControl*		fEndDurationTC;

			BSlider*			fVideoQualitySlider;
			BSlider*			fAudioQualitySlider;

			int32				fVideoQuality;
			int32				fAudioQuality;

			BFilePanel*			fSaveFilePanel;
			BFilePanel*			fOpenFilePanel;

			BDirectory			fOutputDir;
			bool				fOutputDirSpecified;

			bool				fEnabled;
			bool				fConverting;
			bool				fCancelling;
};

#endif // MEDIA_CONVERTER_WINDOW_H


