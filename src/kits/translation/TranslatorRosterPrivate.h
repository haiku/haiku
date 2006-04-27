/*
 * Copyright 2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Author:
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef TRANSLATOR_ROSTER_PRIVATE_H
#define TRANSLATOR_ROSTER_PRIVATE_H


#include <Entry.h>
#include <Handler.h>
#include <Locker.h>
#include <Messenger.h>
#include <TranslatorRoster.h>

#include <map>
#include <vector>

struct translator_data;


struct translator_item {
	BTranslator*	translator;
	entry_ref		ref;
	image_id		image;
};

typedef std::map<translator_id, translator_item> TranslatorMap;
typedef std::vector<BMessenger> MessengerList;

class BTranslatorRoster::Private : public BHandler, public BLocker {
	public:
		Private();
		virtual ~Private();

		virtual void MessageReceived(BMessage* message);

		void AddDefaultPaths();
		status_t AddPaths(const char* paths);
		status_t AddPath(const char* path, int32* _added = NULL);
		status_t AddTranslator(BTranslator* translator, image_id image = -1,
					const entry_ref* ref = NULL);

		BTranslator* FindTranslator(translator_id id);

		status_t StoreTranslators(BMessage& archive);
		status_t Identify(BPositionIO* source, BMessage* ioExtension,
					uint32 hintType, const char* hintMIME, uint32 wantType,
					translator_info* _info);

		status_t GetTranslators(BPositionIO* source, BMessage* ioExtension,
					uint32 hintType, const char* hintMIME, uint32 wantType,
					translator_info** _info, int32* _numInfo);
		status_t GetAllTranslators(translator_id** _ids, int32* _count);
		status_t GetRefFor(translator_id id, entry_ref& ref);

		bool IsActive() const { return Looper(); }

		status_t CreateTranslators(const entry_ref& ref, int32& count);
		status_t GetTranslatorData(image_id image, translator_data& data);

		status_t StartWatching(BMessenger target);
		status_t StopWatching(BMessenger target);

		void TranslatorDeleted(translator_id id);

	private:
		static int _CompareSupport(const void* _a, const void* _b);

		const translation_format* _CheckHints(const translation_format* formats,
					int32 formatsCount, uint32 hintType, const char* hintMIME);
		const translator_item* _FindTranslator(translator_id id) const;
		const translator_item* _FindTranslator(const char* name) const;

		TranslatorMap	fTranslators;
		MessengerList	fMessengers;
		int32			fNextID;
};

#endif	// TRANSLATOR_ROSTER_PRIVATE_H
