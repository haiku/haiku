/*
 * Copyright 2002-2009, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _TRANSLATOR_ROSTER_H
#define _TRANSLATOR_ROSTER_H


#include <Archivable.h>
#include <TranslationDefs.h>


struct translation_format;

class BBitmap;
class BMessage;
class BMessenger;
class BPositionIO;
class BQuery;
class BRect;
class BTranslator;
class BView;
struct entry_ref;


class BTranslatorRoster : public BArchivable {
public:
								BTranslatorRoster();
								BTranslatorRoster(BMessage* model);
	virtual						~BTranslatorRoster();

	static	BTranslatorRoster*	Default();

	virtual	status_t			Archive(BMessage* into, bool deep = true) const;
	static	BArchivable*		Instantiate(BMessage* from);

			status_t			AddTranslators(const char* loadPath = NULL);
			status_t			AddTranslator(BTranslator* translator);

	virtual	status_t			Identify(BPositionIO* source,
									BMessage* ioExtension,
									translator_info* _info, uint32 hintType = 0,
									const char* hintMIME = NULL,
									uint32 wantType = 0);

	virtual	status_t			GetTranslators(BPositionIO* source,
									BMessage* ioExtension,
									translator_info** _info, int32* _numInfo,
									uint32 hintType = 0,
									const char* hintMIME = NULL,
									uint32 wantType = 0);

	virtual	status_t			GetAllTranslators(translator_id** _list,
									int32* _count);

	virtual	status_t			GetTranslatorInfo(translator_id translatorID,
									const char** _name, const char** _info,
									int32* _version);

	virtual	status_t			GetInputFormats(translator_id translatorID,
									const translation_format** _formats,
									int32* _numFormats);

	virtual	status_t			GetOutputFormats(translator_id translatorID,
									const translation_format** _formats,
									int32* _numFormats);

	virtual	status_t			Translate(BPositionIO* source,
									const translator_info* info,
									BMessage* ioExtension,
									BPositionIO* destination,
									uint32 wantOutType, uint32 hintType = 0,
									const char* hintMIME = NULL);

	virtual	status_t			Translate(translator_id translatorID,
									BPositionIO* source, BMessage* ioExtension,
									BPositionIO* destination,
									uint32 wantOutType);

	virtual	status_t			MakeConfigurationView(
									translator_id translatorID,
									BMessage* ioExtension, BView** _view,
									BRect* _extent);

	virtual	status_t			GetConfigurationMessage(
									translator_id translatorID,
									BMessage* ioExtension);

			status_t			GetRefFor(translator_id translatorID,
									entry_ref* ref);
			bool				IsTranslator(entry_ref* ref);

			status_t			StartWatching(BMessenger target);
			status_t			StopWatching(BMessenger target);

			class Private;

private:
			// unimplemented
								BTranslatorRoster(
									const BTranslatorRoster& other);
			BTranslatorRoster&	operator=(const BTranslatorRoster& other);

	virtual	void					ReservedTranslatorRoster1();
	virtual	void					ReservedTranslatorRoster2();
	virtual	void					ReservedTranslatorRoster3();
	virtual	void					ReservedTranslatorRoster4();
	virtual	void					ReservedTranslatorRoster5();
	virtual	void					ReservedTranslatorRoster6();

			void					_Initialize();

	static	const char*				Version(int32* outCurVersion,
										int32* outMinVersion,
										int32 inAppVersion);
				// for backward compatiblity only

private:
			friend class Private;

			Private*				fPrivate;
			int32					fUnused[6];

	static	BTranslatorRoster*		sDefaultRoster;
};


#endif	// _TRANSLATOR_ROSTER_H
