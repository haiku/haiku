/*****************************************************************************/
//               File: TranslatorRoster.h
//              Class: BTranslatorRoster
//   Reimplimented by: Michael Wilber, Translation Kit Team
//   Reimplimentation: 2002-06-11
//
// Description: This class is the guts of the translation kit, it makes the
//              whole thing happen. It bridges the applications using this
//              object with the translators that the apps need to access.
//
//
// Copyright (c) 2002 OpenBeOS Project
//
// Original Version: Copyright 1998, Be Incorporated, All Rights Reserved.
//                   Copyright 1995-1997, Jon Watte
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense, 
// and/or sell copies of the Software, and to permit persons to whom the 
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included 
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
/*****************************************************************************/

#ifndef _TRANSLATOR_ROSTER_H
#define _TRANSLATOR_ROSTER_H

#include <TranslationDefs.h>
#include <OS.h>
#include <StorageDefs.h>
#include <InterfaceDefs.h>
#include <Archivable.h>
#include <TranslatorFormats.h>
#include <TranslationDefs.h>
#include <TranslationErrors.h>
#include <Translator.h>
#include <String.h>
#include <image.h>
#include <File.h>
#include <Directory.h>
#include <Volume.h>
#include <string.h>
#include <OS.h>
#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <alloca.h>
#include <R4xTranslator.h>

struct translation_format;

class BBitmap;
class BView;
class BPositionIO;
class BQuery;
class BMessage;

const char kgDefaultTranslatorPath[] = "/boot/home/config/add-ons/Translators:/boot/home/config/add-ons/Datatypes:/system/add-ons/Translators";

// used by BTranslatorRoster to store the list of translators
struct translator_node {
	BTranslator *translator;
	char *path;
	image_id image;
	translator_id id;
	translator_node *next;
};

class BTranslatorRoster : public BArchivable {
	// private, unimplemented functions
	// (I'm not sure if there is a need for them anyway)
	BTranslatorRoster(const BTranslatorRoster &);
	BTranslatorRoster & operator=(const BTranslatorRoster &);

public:
	BTranslatorRoster();
		// initializes the object with no translators
		
	BTranslatorRoster(BMessage *model);
		// initializes the object and loads all translators from the
		// "be:translator_path" string array in model
		
	~BTranslatorRoster();
		// frees memory used by this object

	virtual status_t Archive(BMessage *into, bool deep = true) const;
		// store the BTranslatorRoster in into so it can later be
		// used with Instantiate or the BMessage constructor
		
	static BArchivable *Instantiate(BMessage *from);
		// creates a BTranslatorRoster object from the from archive

	static const char *Version(int32 *outCurVersion, int32 *outMinVersion,
		int32 inAppVersion = B_TRANSLATION_CURRENT_VERSION);
		// returns the version of BTranslator roster as a string
		// and through the int32 pointers; inAppVersion appears
		// to be ignored

	static BTranslatorRoster *Default();
		// returns the "default" set of translators, they are translators
		// that are loaded from the default paths
		//	/boot/home/config/add-ons/Translators, 
		//	/boot/home/config/add-ons/Datatypes,
		//	/system/add-ons/Translators or the paths in the
		// TRANSLATORS environment variable, if it exists

	status_t AddTranslators(const char *load_path = NULL);
		// this adds a translator, all of the translators in a folder or
		// the default translators if the path is NULL

	status_t AddTranslator(BTranslator * translator);
		// adds a translator that you've created yourself to the
		// BTranslatorRoster; this is useful if you have translators
		// that you don't want to make public or don't want loaded as
		// an add-on

	virtual status_t Identify(BPositionIO *inSource, BMessage *ioExtension,
		translator_info *outInfo, uint32 inHintType = 0,
		const char *inHintMIME = NULL, uint32 inWantType = 0);
		// identifies the translator that can handle the data in inSource

	virtual status_t GetTranslators(BPositionIO *inSource,
		BMessage *ioExtension, translator_info **outInfo, int32 *outNumInfo, 
		uint32 inHintType = 0, const char *inHintMIME = NULL,
		uint32 inWantType = 0);
		// finds all translators for the given type

	virtual status_t GetAllTranslators(translator_id **outList,
		int32 *outCount);
		// returns all translators that this object contains

	virtual	status_t GetTranslatorInfo(translator_id forTranslator,
		const char **outName, const char **outInfo, int32 *outVersion);
		// gets user visible info about a specific translator

	virtual status_t GetInputFormats(translator_id forTranslator,
		const translation_format **outFormats, int32 *outNumFormats);
		// finds all input formats for a translator;
		// note that translators don't always publish the formats
		// that they support

	virtual	status_t GetOutputFormats(translator_id forTranslator,
		const translation_format **outFormats, int32 *outNumFormats);
		// finds all output formats for a translator;
		// note that translators don't always publish the formats
		// that they support

	virtual	status_t Translate(BPositionIO *inSource,
		const translator_info *inInfo, BMessage *ioExtension,
		BPositionIO *outDestination, uint32 inWantOutType,
		uint32 inHintType = 0, const char *inHintMIME = NULL);
		// the whole point of this entire kit boils down to this function;
		// this translates the data in inSource into outDestination
		// using the format inWantOutType

	virtual	status_t Translate(translator_id inTranslator,
		BPositionIO *inSource, BMessage *ioExtension,
		BPositionIO *outDestination, uint32 inWantOutType);
		// the whole point of this entire kit boils down to this function;
		// this translates the data in inSource into outDestination
		// using the format inWantOutType and the translator inTranslator

	virtual status_t MakeConfigurationView(translator_id forTranslator,
		BMessage *ioExtension, BView **outView, BRect *outExtent);
		// create the view for forTranslator that allows the user
		// to configure it;
		// NOTE: translators are not required to support this function

	virtual	status_t GetConfigurationMessage(translator_id forTranslator,
		BMessage *ioExtension);
		// this is used to save the settings for a translator so that
		// they can be written to disk or used in an Indentify or
		// Translate call

	status_t GetRefFor(translator_id translator, entry_ref *out_ref);
		// I'm guessing that this returns the entry_ref for the
		// translator add-on file for the translator_id translator

private:
	void Initialize();
		// class initialization code used by all constructors
		
	translator_node *FindTranslatorNode(translator_id id);
		// used to find the translator_node for the translator_id id
		// returns NULL if the id is not valid

	status_t LoadTranslator(const char *path);
		// loads the translator add-on specified by path
		
	void LoadDir(const char *path, int32 &loadErr, int32 &nLoaded);
		// loads all of the translator add-ons from path and
		// returns error status in loadErr and the number of
		// translators loaded in nLoaded
		
	bool CheckFormats(const translation_format *inputFormats,
		int32 inputFormatsCount, uint32 hintType, const char *hintMIME,
		const translation_format **outFormat);
		// determines how the Identify function is called

	// adds the translator to the list of translators
	// that this object maintains
	status_t AddTranslatorToList(BTranslator *translator);
	status_t AddTranslatorToList(BTranslator *translator,
			const char *path, image_id image, bool acquire);

	static	BTranslatorRoster *fspDefaultTranslators;
		// object that contains the default translators
	translator_node *fpTranslators;
		// list of translators maintained by this object
	sem_id fSem;
		// semaphore used to lock this object
		
	// used to maintain binary combatibility with
	// past and future versions of this object
	int32 fUnused[5];
	virtual	void ReservedTranslatorRoster1();
	virtual	void ReservedTranslatorRoster2();
	virtual	void ReservedTranslatorRoster3();
	virtual	void ReservedTranslatorRoster4();
	virtual	void ReservedTranslatorRoster5();
	virtual	void ReservedTranslatorRoster6();
};

#endif	/* _TRANSLATOR_ROSTER_H */

