/*****************************************************************************/
//               File: TranslatorRoster.cpp
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

#include <TranslatorRoster.h>

// Initialize static member variable
BTranslatorRoster *BTranslatorRoster::fspDefaultTranslators = NULL;

// Extensions used in the extension BMessage, defined in TranslatorFormats.h
char B_TRANSLATOR_EXT_HEADER_ONLY[]			= "/headerOnly";
char B_TRANSLATOR_EXT_DATA_ONLY[]			= "/dataOnly";
char B_TRANSLATOR_EXT_COMMENT[]				= "/comment";
char B_TRANSLATOR_EXT_TIME[]				= "/time";
char B_TRANSLATOR_EXT_FRAME[]				= "/frame";
char B_TRANSLATOR_EXT_BITMAP_RECT[]			= "bits/Rect";
char B_TRANSLATOR_EXT_BITMAP_COLOR_SPACE[]	= "bits/space";
char B_TRANSLATOR_EXT_BITMAP_PALETTE[]		= "bits/palette";
char B_TRANSLATOR_EXT_SOUND_CHANNEL[]		= "nois/channel";
char B_TRANSLATOR_EXT_SOUND_MONO[]			= "nois/mono";
char B_TRANSLATOR_EXT_SOUND_MARKER[]		= "nois/marker";
char B_TRANSLATOR_EXT_SOUND_LOOP[]			= "nois/loop";

// ---------------------------------------------------------------
// Constructor
//
// Private, unimplimented constructor that no one should use.
// I don't think there would be much of a need for this function.
//
// Preconditions:
//
// Parameters: tr, not used
//
// Postconditions:
//
// Returns:
// ---------------------------------------------------------------
BTranslatorRoster::BTranslatorRoster(const BTranslatorRoster &tr)
	: BArchivable()
{
	Initialize();
}

// ---------------------------------------------------------------
// operator=
//
// Private, unimplimented function that no one should use.
// I don't know that there is a need for this function anyway.
//
// Preconditions:
//
// Parameters: tr, not used
//
// Postconditions:
//
// Returns: refernce to this object
// ---------------------------------------------------------------
BTranslatorRoster &
BTranslatorRoster::operator=(const BTranslatorRoster &tr)
{
	return *this;
}

// ---------------------------------------------------------------
// Constructor
//
// Initilizes the BTranslatorRoster to the empty state, it loads
// no translators.
//
// Preconditions:
//
// Parameters:
//
// Postconditions:
//
// Returns:
// ---------------------------------------------------------------
BTranslatorRoster::BTranslatorRoster() : BArchivable()
{
	Initialize();
}

// ---------------------------------------------------------------
// Constructor
//
// This constructor initilizes the BTranslatorRoster, then
// loads all of the translators specified in the
// "be:translator_path" field of the supplied BMessage.
//
// Preconditions:
//
// Parameters: model, the BMessage where the translator paths are
//                    found.
//
// Postconditions:
//
// Returns:
// ---------------------------------------------------------------
BTranslatorRoster::BTranslatorRoster(BMessage *model) : BArchivable()
{
	Initialize();
	
	if (model) {
		BString bstr;
		for (int32 i = 0;
			model->FindString("be:translator_path", i, &bstr) == B_OK; i++) {
			AddTranslators(bstr.String());
			bstr = "";
		}
	}		
}

// ---------------------------------------------------------------
// Initialize()
//
// This function initializes this object to the empty state. It
// is used by all constructors.
//
// Preconditions: Object must be in initial uninitialized state
//
// Parameters:
//
// Postconditions:
//
// Returns:
// ---------------------------------------------------------------
void BTranslatorRoster::Initialize()
{
	fpTranslators = NULL;
	fSem = create_sem(1, "BTranslatorRoster Lock");
}

// ---------------------------------------------------------------
// Destructor
//
// Unloads any translators that were loaded and frees all memory
// allocated by the BTranslatorRoster, except for the static
// member data.
//
// Preconditions:
//
// Parameters:
//
// Postconditions:
//
// Returns:
// ---------------------------------------------------------------
BTranslatorRoster::~BTranslatorRoster()
{
	if (fSem > 0 && acquire_sem(fSem) == B_NO_ERROR) {
	
		// FIRST PASS: release BTranslator objects
		translator_node *pTranNode = fpTranslators;
		while (pTranNode) {
			translator_node *pRelTranNode = pTranNode;
			pTranNode = pTranNode->next;
			pRelTranNode->translator->Release();
		}
		
		// SECOND PASS: unload images and delete nodes
		// (I can't delete a BTranslator if I've deleted
		// the code for it)
		pTranNode = fpTranslators;
		while (pTranNode) {
			translator_node *pDelTranNode = pTranNode;
			pTranNode = pTranNode->next;
			
			// only try to unload actual images
			if (pDelTranNode->image >= 0)
				unload_add_on(pDelTranNode->image);
					// I may end up trying to unload the same image
					// more than once, but I don't think
					// that should be a problem
			delete[] pDelTranNode->path;
			delete pDelTranNode;
		}		
		
		fpTranslators = NULL;
	}
	
	delete_sem(fSem);
}

// ---------------------------------------------------------------
// Archive
//
// Archives the BTranslatorRoster by recording its loaded add-ons
// in the BMessage into. The deep variable appeared to have no
// impact in Be's version of this function, so I went the same
// route.
//
// Preconditions:
//
// Parameters: into, the BMessage that this object is written to
//             deep, if true, more data is written, if false,
//                   less data is written
//
// Postconditions:
//
// Returns: B_OK, if everything went well
//          B_BAD_VALUE, if into is NULL
//          B_NOT_INITIALIZED, if the constructor couldn't create
//                             a semaphore
//          other errors that BMessage::AddString() and
//          BArchivable::Archive() return
// ---------------------------------------------------------------
status_t
BTranslatorRoster::Archive(BMessage *into, bool deep) const
{
	if (!into)
		return B_BAD_VALUE;

	status_t result = B_NOT_INITIALIZED;

	if (fSem > 0 && acquire_sem(fSem) == B_OK) {
		result = BArchivable::Archive(into, deep);
		if (result == B_OK) {
			result = into->AddString("class", "BTranslatorRoster");
			
			translator_node *pTranNode = NULL;
			for (pTranNode = fpTranslators; result == B_OK && pTranNode;
				pTranNode = pTranNode->next) {
				if (pTranNode->path[0])
					result = into->AddString("be:translator_path",
						pTranNode->path);
			}
		}
		release_sem(fSem);
	}
	
	return result;
}

// ---------------------------------------------------------------
// Instantiate
//
// This static member function returns a new BTranslatorRosters
// object, allocated by new and created with the version of the
// constructor that takes a BMessage archive. However if the
// archive doesn't contain data for a BTranslatorRoster object,
// Instantiate() returns NULL.
//
// Preconditions:
//
// Parameters: from, the BMessage to create the new object from
//
// Postconditions:
//
// Returns: returns NULL if the BMessage was no good
//          returns a BArchivable * to a BTranslatorRoster
// ---------------------------------------------------------------
BArchivable *
BTranslatorRoster::Instantiate(BMessage *from)
{
	if (!from || !validate_instantiation(from, "BTranslatorRoster"))
		return NULL;
	else
		return new BTranslatorRoster(from);
}

// ---------------------------------------------------------------
// Version
//
// Sets outCurVersion to the Translation Kit protocol version
// number and outMinVersion to the minimum  protocol version number
// supported. Returns a string containing verbose version
// information.  Currently, inAppVersion must be 
// B_TRANSLATION_CURRENT_VERSION, but as far as I can tell, its
// completely ignored.
//
// Preconditions:
//
// Parameters: outCurVersion, the current version is stored here
//             outMinVersion, the minimum supported version is
//                            stored here
//             inAppVersion, is ignored as far as I know
//
// Postconditions:
//
// Returns: string of verbose translation kit version
//          information or an empty string if either of the
//          out variables is NULL.
// ---------------------------------------------------------------
const char *
BTranslatorRoster::Version(int32 *outCurVersion, int32 *outMinVersion,
	int32 inAppVersion)
{
	if (!outCurVersion || !outMinVersion)
		return "";

	static char vString[50];
	static char vDate[] = __DATE__;
	if (!vString[0]) {
		sprintf(vString, "Translation Kit v%d.%d.%d %s\n",
			B_TRANSLATION_CURRENT_VERSION/100,
			(B_TRANSLATION_CURRENT_VERSION/10)%10,
			B_TRANSLATION_CURRENT_VERSION%10,
			vDate);
	}
	*outCurVersion = B_TRANSLATION_CURRENT_VERSION;
	*outMinVersion = B_TRANSLATION_MIN_VERSION;
	return vString;
}

// ---------------------------------------------------------------
// Default
//
// This static member function returns a pointer to the default
// BTranslatorRoster that has the default translators stored in 
// it. The paths for the default translators are loaded from
// the TRANSLATORS environment variable. If it does not exist,
// the paths used are /boot/home/config/add-ons/Translators,
// /boot/home/config/add-ons/Datatypes, and
// /system/add-ons/Translators. The BTranslatorRoster returned
// by this function is global to the application and should
// not be deleted.
//
// This code probably isn't thread safe (yet).
//
// Preconditions:
//
// Parameters:
//
// Postconditions:
//
// Returns: pointer to the default BTranslatorRoster
// ---------------------------------------------------------------
BTranslatorRoster *
BTranslatorRoster::Default()
{
	// If the default translators have not been loaded,
	// create a new BTranslatorRoster for them, and load them.
	if (!fspDefaultTranslators) {
		fspDefaultTranslators = new BTranslatorRoster();
		fspDefaultTranslators->AddTranslators(NULL);
	}
	
	return fspDefaultTranslators;
}

// ---------------------------------------------------------------
// AddTranslators
//
// This function takes a string of colon delimited paths, 
// (folders or specific files) and adds the translators from
// those paths to this BTranslatorRoster.
//
// If load_path is NULL, it parses the environment variable
// TRANSLATORS. If that does not exist, it uses the paths:
// /boot/home/config/add-ons/Translators,
// /boot/home/config/add-ons/Datatypes and
// /system/add-ons/Translators.
//
// Preconditions:
//
// Parameters: load_path, colon delimited list of paths
//                        to load translators from
//
// Postconditions:
//
// Returns: B_OK on success,
//          B_BAD_VALUE if there was something wrong with
//                      load_path
//          other errors for problems with loading add-ons
// ---------------------------------------------------------------
status_t
BTranslatorRoster::AddTranslators(const char *load_path)
{
	if (fSem <= 0)
		return fSem;
		
	status_t loadErr = B_ERROR;
	int32 nLoaded = 0;

	if (acquire_sem(fSem) == B_OK) {
		if (load_path == NULL)
			load_path = getenv("TRANSLATORS");
		if (load_path == NULL)
			load_path = kgDefaultTranslatorPath;
			
		char pathbuf[PATH_MAX];
		const char *ptr = load_path;
		const char *end = ptr;
		struct stat stbuf;
		while (*ptr != 0) {
			//	find segments specified by colons
			end = strchr(ptr, ':');
			if (end == NULL)
				end = ptr + strlen(ptr);
			if (end-ptr > PATH_MAX - 1)
				loadErr = B_BAD_VALUE;
			else {
				//	copy this segment of the path into a path, and load it
				memcpy(pathbuf, ptr, end - ptr);
				pathbuf[end - ptr] = 0;
	
				if (!stat(pathbuf, &stbuf)) {
					//	files are loaded as translators
					if (S_ISREG(stbuf.st_mode)) {
						status_t err = LoadTranslator(pathbuf);
						if (err != B_OK)
							loadErr = err;
						else
							nLoaded++;
					} else
						//	directories are scanned
						LoadDir(pathbuf, loadErr, nLoaded);
				}
			}
			ptr = end + 1;
			if (*end == 0)
				break;
		} // while (*ptr != 0)
		
		release_sem(fSem);
		
	} // if (acquire_sem(fSem) == B_OK)

	//	if anything loaded, it's not too bad
	if (nLoaded)
		loadErr = B_OK;

	return loadErr;
}

// ---------------------------------------------------------------
// AddTranslator
//
// Adds a BTranslator based object to the BTranslatorRoster.
// When you add a BTranslator roster, it is Acquire()'d by
// BTranslatorRoster; it is Release()'d when the
// BTranslatorRoster is deleted.
//
// Preconditions:
//
// Parameters: translator, the translator to be added to the
//                         BTranslatorRoster
//
// Postconditions:
//
// Returns: B_BAD_VALUE, if translator is NULL,
//          B_NOT_INITIALIZED, if the constructor couldn't
//                             create a semaphore
//          B_OK if all went well 
// ---------------------------------------------------------------
status_t
BTranslatorRoster::AddTranslator(BTranslator *translator)
{
	if (!translator)
		return B_BAD_VALUE;

	status_t result = B_NOT_INITIALIZED;
		
	if (fSem > 0 && acquire_sem(fSem) == B_NO_ERROR) {
		// Determine if translator is already in the list
		translator_node *pNode = fpTranslators;
		while (pNode) {
			// If translator is in the list, don't add it
			if (!strcmp(pNode->translator->TranslatorName(),
				translator->TranslatorName())) {
				result = B_OK;
				break;
			}
			pNode = pNode->next;
		}
		
		// if translator is NOT in the list, add it
		if (!pNode)
			result = AddTranslatorToList(translator);
		
		release_sem(fSem);
	}

	return result;
}

// ---------------------------------------------------------------
// Identify
//
// This function determines which translator is best suited
// to convert the data from inSource. 
//
// Preconditions:
//
// Parameters: inSource, the data to be translated,
//             ioExtension, the configuration data for the
//                          translator
//             outInfo, the information about the chosen
//                      translator is put here
//             inHintType, a hint about the type of data
//                         that is in inSource, can be
//                         zero if type is not known
//             inHintMIME, a hint about the MIME type of
//                         data that is in inSource, 
//                         can be NULL
//             inWantType, the desired output type for
//                         inSource, if zero, any type
//                         is ok.
//
// Postconditions:
//
// Returns: B_OK, identification of inSource was successful,
//          B_NO_TRANSLATOR, no appropriate translator found
//          B_NOT_INITIALIZED, the constructor failed to 
//                             create a semaphore
//          B_BAD_VALUE, inSource or outInfo is NULL
//          other values, error using inSource
// ---------------------------------------------------------------
status_t
BTranslatorRoster::Identify(BPositionIO *inSource,
	BMessage *ioExtension, translator_info *outInfo,
	uint32 inHintType, const char *inHintMIME,
	uint32 inWantType)
{
	if (!inSource || !outInfo)
		return B_BAD_VALUE;

	status_t result = B_NOT_INITIALIZED;
	bool bFoundMatch = false;
	
	if (fSem > 0 && acquire_sem(fSem) == B_OK) {
	
		translator_node *pTranNode = fpTranslators;
		float bestWeight = 0.0;
		for (; pTranNode; pTranNode = pTranNode->next) {

			const translation_format *format = NULL;
			translator_info tmpInfo;
			float weight = 0.0;
			bool addmatch = false;
				// eliminates need for a goto
				
			result = inSource->Seek(0, SEEK_SET);
			if (result == B_OK) {
			
				int32 inputFormatsCount = 0;
				const translation_format *inputFormats =
					pTranNode->translator->InputFormats(&inputFormatsCount);
				
				if (CheckFormats(inputFormats, inputFormatsCount, inHintType,
					 inHintMIME, &format)) {

					// after checking the formats for hints, we still need to make
					// sure the translator recognizes the data and can output the
					// desired format, so we call its' Identify() function.
					if (format && !pTranNode->translator->Identify(inSource,
						format, ioExtension, &tmpInfo, inWantType))
						addmatch = true;
					
				} else if (!pTranNode->translator->Identify(inSource, NULL,
					ioExtension, &tmpInfo, inWantType))
					addmatch = true;
				
				if (addmatch) {
					weight = tmpInfo.quality * tmpInfo.capability;
					
					// Since many kinds of data can look like text,
					// don't choose the text format if it has been identified
					// as belonging to a different group.
					if (bFoundMatch && outInfo->group == B_TRANSLATOR_TEXT &&
						tmpInfo.group != B_TRANSLATOR_TEXT)
						bestWeight = 0.0;
					else if (bFoundMatch && tmpInfo.group == B_TRANSLATOR_TEXT &&
						outInfo->group != B_TRANSLATOR_TEXT)
						weight = 0.0;

					if (weight > bestWeight) {
						bFoundMatch = true;
						bestWeight = weight;
						
						tmpInfo.translator = pTranNode->id;
						outInfo->type = tmpInfo.type;
						outInfo->translator = tmpInfo.translator;
						outInfo->group = tmpInfo.group;
						outInfo->quality = tmpInfo.quality;
						outInfo->capability = tmpInfo.capability;
						strcpy(outInfo->name, tmpInfo.name);
						strcpy(outInfo->MIME, tmpInfo.MIME);
					}
				}
			} // if (result == B_OK)
		} // for (; pTranNode; pTranNode = pTranNode->next)
		
		if (bFoundMatch)
			result = B_NO_ERROR;
		else if (result == B_OK)
			result = B_NO_TRANSLATOR;
		
		release_sem(fSem);
	} // if (acquire_sem(fSem) == B_OK)
	
	return result;
}

// ---------------------------------------------------------------
// compare_data
//
// This function is not a member of BTranslatorRoster, but it is
// used by the GetTranslators() member function as the function
// passed to qsort to sort the translators from best to worst.
//
// Preconditions:
//
// Parameters: a, pointer to translator_info structure to be
//                compared to b
//             b, pointer to translator_info structure to be
//                compared to a
//
// Postconditions:
//
// Returns: < 0 if a is less desirable than b, 
//          0 if a as good as b,
//          > 0 if a is more desirable than b
// ---------------------------------------------------------------
static int
compare_data(const void *a, const void *b)
{
	register translator_info *ai = (translator_info *)a;
	register translator_info *bi = (translator_info *)b;
	return (int) (- ai->quality * ai->capability +
		bi->quality * bi->capability);
}

// ---------------------------------------------------------------
// GetTranslators
//
// Finds all translators capable of handling the data in inSource
// and puts them into the outInfo array (which you must delete
// yourself when you are done with it). Specifying a value for
// inHintType, inHintMIME and/or inWantType causes only the
// translators that satisfy them to be included in the outInfo.
//
// Preconditions:
//
// Parameters: inSource, the data that wants to be translated
//             ioExtension, configuration data for the translator
//             outInfo, the array of acceptable translators is
//                      stored here if the function succeeds
//             outNumInfo, number of entries in outInfo
//             inHintType, hint for the type of data in 
//                         inSource, can be zero if the
//                         type is not known
//             inHintMIME, hint MIME type of the data
//                         in inSource, can be NULL if
//                         the MIME type is not known
//             inWantType, the desired output type for
//                         the data in inSource, can be zero
//                         for any type.
//
// Postconditions:
//
// Returns: B_OK, successfully indentified the data in inSource
//          B_NO_TRANSLATOR, no translator could handle inSource
//          B_NOT_INITIALIZED, the constructore failed to create
//                              a semaphore
//          B_BAD_VALUE, inSource, outInfo, or outNumInfo is NULL
//          other errors, problems using inSource
// ---------------------------------------------------------------
status_t
BTranslatorRoster::GetTranslators(BPositionIO *inSource,
	BMessage *ioExtension, translator_info **outInfo,
	int32 *outNumInfo, uint32 inHintType,
	const char *inHintMIME, uint32 inWantType)
{
	if (!inSource || !outInfo || !outNumInfo)
		return B_BAD_VALUE;

	status_t result = B_NOT_INITIALIZED;
	
	*outInfo = NULL;
	*outNumInfo = 0;

	if (fSem > 0 && acquire_sem(fSem) == B_OK) {

		int32 physCnt = 10;
		*outInfo = new translator_info[physCnt];
		*outNumInfo = 0;

		translator_node *pTranNode = fpTranslators;
		for (; pTranNode; pTranNode = pTranNode->next) {

			const translation_format *format = NULL;
			translator_info tmpInfo;
			bool addmatch = false;
				// avoid the need for a goto

			result = inSource->Seek(0, SEEK_SET);
			if (result < B_OK) {
				// break out of the loop if error reading from source
				delete *outInfo;
				*outInfo = NULL;
				break;
			} else {
				int32 inputFormatsCount = 0;
				const translation_format *inputFormats =
					pTranNode->translator->InputFormats(&inputFormatsCount);
				
				if (CheckFormats(inputFormats, inputFormatsCount, inHintType,
					inHintMIME, &format)) {
					if (format && !pTranNode->translator->Identify(inSource,
						format, ioExtension, &tmpInfo, inWantType))
						addmatch = true;
					
				} else if (!pTranNode->translator->Identify(inSource, NULL,
					ioExtension, &tmpInfo, inWantType))
					addmatch = true;

				if (addmatch) {
					//	dynamically resize output list
					//
					if (physCnt <= *outNumInfo) {
						physCnt += 10;
						translator_info *nOut = new translator_info[physCnt];
						for (int ix = 0; ix < *outNumInfo; ix++)
							nOut[ix] = (*outInfo)[ix];
						
						delete[] *outInfo;
						*outInfo = nOut;
					}

					 // XOR to discourage taking advantage of undocumented
					 // features
					tmpInfo.translator = pTranNode->id;
					(*outInfo)[(*outNumInfo)++] = tmpInfo;
				}
			}
		} // for (; pTranNode; pTranNode = pTranNode->next)
		
		// if exited loop WITHOUT errors
		if (!pTranNode) {
		
			if (*outNumInfo > 1)
				qsort(*outInfo, *outNumInfo, sizeof(**outInfo), compare_data);
				
			if (*outNumInfo > 0)
				result = B_NO_ERROR;
			else
				result = B_NO_TRANSLATOR;
		}
			
		release_sem(fSem);
		
	} // if (acquire_sem(fSem) == B_OK)
	
	return result;
}

// ---------------------------------------------------------------
// GetAllTranslators
//
// Returns a list of all of the translators stored by this object.
// You must delete the list, outList, yourself when you are done
// with it.
//
// Preconditions:
//
// Parameters: outList, where the list is stored,
//                      (you must delete the list yourself)
//             outCount, where the count of the items in
//                       the list is stored
//
// Postconditions:
//
// Returns: B_BAD_VALUE, if outList or outCount is NULL
//          B_NOT_INITIALIZED, if the constructor couldn't
//                             create a semaphore
//          B_NO_ERROR, if successful
// ---------------------------------------------------------------
status_t
BTranslatorRoster::GetAllTranslators(
	translator_id **outList, int32 *outCount)
{
	if (!outList || !outCount)
		return B_BAD_VALUE;

	status_t result = B_NOT_INITIALIZED;
	
	*outList = NULL;
	*outCount = 0;

	if (fSem > 0 && acquire_sem(fSem) == B_OK) {
		// count handlers
		translator_node *pTranNode = NULL;
		for (pTranNode = fpTranslators; pTranNode; pTranNode = pTranNode->next)
			(*outCount)++;
		*outList = new translator_id[*outCount];
		*outCount = 0;
		for (pTranNode = fpTranslators; pTranNode; pTranNode = pTranNode->next)
			(*outList)[(*outCount)++] = pTranNode->id;
			
		result = B_NO_ERROR;
		release_sem(fSem);
	} 

	return result;
}

// ---------------------------------------------------------------
// GetTranslatorInfo
//
// Returns information about the translator with translator_id
// forTranslator. outName is the short name of the translator,
// outInfo is the verbose name / description of the translator,
// and outVersion is the integer representation of the translator
// version.
//
// Preconditions:
//
// Parameters: forTranslator, identifies which translator
//                            you want info for
//             outName, the translator name is put here
//             outInfo, the translator info is put here
//             outVersion, the translation version is put here
//
// Postconditions:
//
// Returns: B_NO_ERROR, if successful,
//          B_BAD_VALUE, if any parameter is NULL
//          B_NOT_INITIALIZED, if the constructor couldn't
//                             create a semaphore
//          B_NO_TRANSLATOR, if forTranslator is not a valid
//                           translator id
// ---------------------------------------------------------------
status_t
BTranslatorRoster::GetTranslatorInfo(
	translator_id forTranslator, const char **outName,
	const char **outInfo, int32 *outVersion)
{
	if (!outName || !outInfo || !outVersion)
		return B_BAD_VALUE;

	status_t result = B_NOT_INITIALIZED;
	
	if (fSem > 0 && acquire_sem(fSem) == B_OK) {
		// find the translator we've requested
		translator_node *pTranNode = FindTranslatorNode(forTranslator);
		if (!pTranNode)
			result = B_NO_TRANSLATOR;
		else {
			*outName = pTranNode->translator->TranslatorName();
			*outInfo = pTranNode->translator->TranslatorInfo();
			*outVersion = pTranNode->translator->TranslatorVersion();
			
			result = B_NO_ERROR;
		}
		release_sem(fSem);
	}

	return result;
}

// ---------------------------------------------------------------
// GetInputFormats
//
// Returns all of the input formats for the translator
// forTranslator. Not all translators publish the input formats
// that they accept.
//
// Preconditions:
//
// Parameters: forTranslator, identifies which translator
//                            you want info for
//              outFormats, array of input formats
//              outNumFormats, number of items in outFormats
//
// Postconditions:
//
// Returns: B_NO_ERROR, if successful
//          B_BAD_VALUE, if either pointer is NULL
//          B_NOT_INITIALIZED, if the constructor couldn't
//                             create a semaphore
//          B_NO_TRANSLATOR, if forTranslator is a bad id
// ---------------------------------------------------------------
status_t
BTranslatorRoster::GetInputFormats(translator_id forTranslator,
	const translation_format **outFormats, int32 *outNumFormats)
{
	if (!outFormats || !outNumFormats)
		return B_BAD_VALUE;

	status_t result = B_NOT_INITIALIZED;
	
	*outFormats = NULL;
	*outNumFormats = 0;

	if (fSem > 0 && acquire_sem(fSem) == B_OK) {
		//	find the translator we've requested
		translator_node *pTranNode = FindTranslatorNode(forTranslator);
		if (!pTranNode)
			result = B_NO_TRANSLATOR;
		else {
			*outFormats = pTranNode->translator->InputFormats(outNumFormats);
			result = B_NO_ERROR;
		}
		release_sem(fSem);
	}
	
	return result;
}

// ---------------------------------------------------------------
// GetOutputFormats
//
// Returns all of the output formats for the translator
// forTranslator. Not all translators publish the output formats
// that they accept.
//
// Preconditions:
//
// Parameters: forTranslator, identifies which translator
//                            you want info for
//              outFormats, array of output formats
//              outNumFormats, number of items in outFormats
//
// Postconditions:
//
// Returns: B_NO_ERROR, if successful
//          B_BAD_VALUE, if either pointer is NULL
//          B_NOT_INITIALIZED, if the constructor couldn't
//                             create a semaphore
//          B_NO_TRANSLATOR, if forTranslator is a bad id
// ---------------------------------------------------------------
status_t
BTranslatorRoster::GetOutputFormats(translator_id forTranslator,
	const translation_format **outFormats, int32 *outNumFormats)
{
	if (!outFormats || !outNumFormats)
		return B_BAD_VALUE;

	status_t result = B_NOT_INITIALIZED;
	
	*outFormats = NULL;
	*outNumFormats = 0;

	if (fSem > 0 && acquire_sem(fSem) == B_OK) {
		// find the translator we've requested
		translator_node *pTranNode = FindTranslatorNode(forTranslator);
		if (!pTranNode)
			result = B_NO_TRANSLATOR;
		else {
			*outFormats = pTranNode->translator->OutputFormats(outNumFormats);
			result = B_NO_ERROR;
		}
		release_sem(fSem);
	}
	
	return result;
}

// ---------------------------------------------------------------
// Translate
//
// This function is the whole point of the Translation Kit.
// This is for translating the data in inSource to outDestination
// using the format inWantOutType.
//
// Preconditions:
//
// Parameters: inSource, the data that wants to be translated
//             ioExtension, configuration data for the translator
//             inInfo, identifies the translator to use, can be
//                     NULL, calls Identify() if NULL
//             inHintType, hint for the type of data in 
//                         inSource, can be zero if the
//                         type is not known
//             inHintMIME, hint MIME type of the data
//                         in inSource, can be NULL if
//                         the MIME type is not known
//             inWantOutType, the desired output type for
//                            the data in inSource.
//             outDestination,  where inSource is translated to
//
// Postconditions:
//
// Returns: B_OK, translation successful,
//          B_NO_TRANSLATOR, no appropriate translator found
//          B_NOT_INITIALIZED, the constructor failed to 
//                             create a semaphore
//          B_BAD_VALUE, inSource or outDestination is NULL
//          other values, error using inSource
// ---------------------------------------------------------------
status_t
BTranslatorRoster::Translate(BPositionIO *inSource,
	const translator_info *inInfo, BMessage *ioExtension,
	BPositionIO *outDestination, uint32 inWantOutType,
	uint32 inHintType, const char *inHintMIME)
{
	if (!inSource || !outDestination)
		return B_BAD_VALUE;
		
	if (fSem <= 0)
		return B_NOT_INITIALIZED;

	status_t result = B_OK;
	translator_info stat_info;

	if (!inInfo) {
		//	go look for a suitable translator
		inInfo = &stat_info;

		result = Identify(inSource, ioExtension, &stat_info, 
				inHintType, inHintMIME, inWantOutType);
			// Identify is a locked function, so it cannot be
			// called from code that is already locked
	} 
	
	if (result >= B_OK && acquire_sem(fSem) == B_OK) {
		translator_node *pTranNode = FindTranslatorNode(inInfo->translator);
		if (!pTranNode) {
			result = B_NO_TRANSLATOR;
		}
		else {
			result = inSource->Seek(0, SEEK_SET);
			if (result == B_OK)
				result = pTranNode->translator->Translate(inSource, inInfo,
					ioExtension, inWantOutType, outDestination);
		}
		release_sem(fSem);
	}

	return result;
}

// ---------------------------------------------------------------
// Translate
//
// This function is the whole point of the Translation Kit.
// This is for translating the data in inSource to outDestination
// using the format inWantOutType.
//
// Preconditions:
//
// Parameters: inSource, the data that wants to be translated
//             ioExtension, configuration data for the translator
//             inTranslator, the translator to use for the 
//                           translation
//             inWantOutType, the desired output type for
//                            the data in inSource.
//             outDestination,  where inSource is translated to
//
// Postconditions:
//
// Returns: B_OK, translation successful,
//          B_NO_TRANSLATOR, inTranslator is an invalid id
//          B_NOT_INITIALIZED, the constructor failed to 
//                             create a semaphore
//          B_BAD_VALUE, inSource or outDestination is NULL
//          other values, error using inSource
// ---------------------------------------------------------------
status_t
BTranslatorRoster::Translate(translator_id inTranslator,
	BPositionIO *inSource, BMessage *ioExtension,
	BPositionIO *outDestination, uint32 inWantOutType)
{	
	if (!inSource || !outDestination)
		return B_BAD_VALUE;

	status_t result = B_NOT_INITIALIZED;
	
	if (fSem > 0 && acquire_sem(fSem) == B_OK) {
		translator_node *pTranNode = FindTranslatorNode(inTranslator);
		if (!pTranNode)
			result = B_NO_TRANSLATOR;
		else {
			result = inSource->Seek(0, SEEK_SET);
			if (result == B_OK) {
				translator_info traninfo;
				result = pTranNode->translator->Identify(inSource, NULL,
					ioExtension, &traninfo, inWantOutType);	
				if (result == B_OK) {
					result = inSource->Seek(0, SEEK_SET);
					if (result == B_OK) {
						result = pTranNode->translator->Translate(inSource,
							&traninfo, ioExtension, inWantOutType,
							outDestination);
					}
				}
			}
		}
		release_sem(fSem);
	}

	return result;
}

// ---------------------------------------------------------------
// MakeConfigurationView
//
// Returns outView, a BView for configuring the translator
// forTranslator. Not all translators support this.
//
// Preconditions:
//
// Parameters: forTranslator, the translator the view is for
//             ioExtension, the configuration data for
//                          the translator
//             outView, the view for configuring the
//                      translator
//             outExtent, the bounds for the view, the view
//                        can be resized
//
// Postconditions:
//
// Returns: B_OK, success,
//          B_NO_TRANSLATOR, inTranslator is an invalid id
//          B_NOT_INITIALIZED, the constructor failed to 
//                             create a semaphore
//          B_BAD_VALUE, inSource or outDestination is NULL
//          other values, error using inSource
// ---------------------------------------------------------------
status_t
BTranslatorRoster::MakeConfigurationView(
	translator_id forTranslator, BMessage *ioExtension,
	BView **outView, BRect *outExtent)
{
	if (!outView || !outExtent)
		return B_BAD_VALUE;

	status_t result = B_NOT_INITIALIZED;

	if (fSem > 0 && acquire_sem(fSem) == B_OK) {
		translator_node *pTranNode = FindTranslatorNode(forTranslator);
		if (!pTranNode)
			result = B_NO_TRANSLATOR;
		else
			result = pTranNode->translator->MakeConfigurationView(ioExtension,
				outView, outExtent);
			
		release_sem(fSem);
	}
	
	return result;
}

// ---------------------------------------------------------------
// GetConfigurationMessage
//
// Gets the configuration setttings for the translator
// forTranslator and puts the settings into ioExtension.
//
// Preconditions:
//
// Parameters: forTranslator, the translator the info
//                            is for
//             ioExtension, the configuration data for
//                          the translator is stored here
//
// Postconditions:
//
// Returns: B_OK, success,
//          B_NO_TRANSLATOR, inTranslator is an invalid id
//          B_NOT_INITIALIZED, the constructor failed to 
//                             create a semaphore
//          B_BAD_VALUE, inSource or outDestination is NULL
//          other values, error using inSource
// ---------------------------------------------------------------
status_t
BTranslatorRoster::GetConfigurationMessage(
	translator_id forTranslator, BMessage *ioExtension)
{
	if (!ioExtension)
		return B_BAD_VALUE;
		
	status_t result = B_NOT_INITIALIZED;

	if (fSem > 0 && acquire_sem(fSem) == B_OK) {
		translator_node *pTranNode = FindTranslatorNode(forTranslator);
		if (!pTranNode)
			result = B_NO_TRANSLATOR;
		else
			result =
				pTranNode->translator->GetConfigurationMessage(ioExtension);
		
		release_sem(fSem);
	}
	
	return result;
}

// ---------------------------------------------------------------
// GetRefFor
//
// Gets the entry_ref for the given translator.
//
// Preconditions:
//
// Parameters: forTranslator, the translator the info
//                            is for
//             entry_ref, where the entry ref is stored
//
// Postconditions:
//
// Returns: B_OK, success,
//          B_NO_TRANSLATOR, translator is an invalid id
//          B_NOT_INITIALIZED, the constructor failed to 
//                             create a semaphore
//          B_BAD_VALUE, inSource or outDestination is NULL
//          other values, error using inSource
// ---------------------------------------------------------------
status_t
BTranslatorRoster::GetRefFor(translator_id translator,
	entry_ref *out_ref)
{
	if (!out_ref)
		return B_BAD_VALUE;

	status_t result = B_NOT_INITIALIZED;
	
	if (fSem > 0 && acquire_sem(fSem) == B_OK) {
		translator_node *pTranNode = FindTranslatorNode(translator);
		if (!pTranNode)
			result = B_NO_TRANSLATOR;
		else {
			if (pTranNode->path[0] == '\0')
				result = B_ERROR;
			else 
				result = get_ref_for_path(pTranNode->path, out_ref);
		}
		release_sem(fSem);
	}
	
	return result;
}


// ---------------------------------------------------------------
// FindTranslatorNode
//
// Finds the translator_node that holds the translator with
// the translator_id id.
//
// Preconditions:
//
// Parameters: id, the translator you want a translator_node for
//
// Postconditions:
//
// Returns: NULL if id is not a valid translator_id,
//          pointer to the translator_node that holds the 
//          translator id
// ---------------------------------------------------------------
translator_node *
BTranslatorRoster::FindTranslatorNode(translator_id id)
{
	translator_node *pTranNode = NULL;
	for (pTranNode = fpTranslators; pTranNode; pTranNode = pTranNode->next)
		if (pTranNode->id == id)
			break;
			
	return pTranNode;
}

// ---------------------------------------------------------------
// LoadTranslator
//
// Loads the translator from path into memory and adds it to
// the BTranslatorRoster.
//
// Preconditions: This should only be called inside of locked 
//                code.
//
// Parameters: path, the path for the translator to be loaded
//
// Postconditions:
//
// Returns: B_BAD_VALUE, if path is NULL,
//          B_NO_ERROR, if all is well,
//          other values if error loading add ons
// ---------------------------------------------------------------
status_t
BTranslatorRoster::LoadTranslator(const char *path)
{
	if (!path)
		return B_BAD_VALUE;

	//	check that this ref is not already loaded
	const char *name = strrchr(path, '/');
	if (name)
		name++;
	else
		name = path;
	for (translator_node *i = fpTranslators; i; i=i->next)
		if (!strcmp(name, i->translator->TranslatorName()))
			return B_NO_ERROR;
			//	we use name for determining whether it's loaded
			//	that is not entirely foolproof, but making SURE will be 
			//	a very slow process that I don't much care for.
			
	image_id image = load_add_on(path);
		// Load the data and code for the Translator into memory
	if (image < 0)
		return image;
	
	// Function pointer used to create post R4.5 style translators
	BTranslator *(*pMakeNthTranslator)(int32 n,image_id you,uint32 flags,...);
	
	status_t err = get_image_symbol(image, "make_nth_translator",
		B_SYMBOL_TYPE_TEXT, (void **)&pMakeNthTranslator);
	if (!err) {
		// If the translator add-on supports the post R4.5
		// translator creation mechanism
		
		BTranslator *ptran = NULL;
		// WARNING: This code assumes that the ref count on the 
		// BTranslators from MakeNth... begin at 1!!!
		// I NEED TO WRITE CODE TO TEST WHAT THE REF COUNT FOR 
		// THESE BTRANSLATORS START AS!!!!
		for (int32 n = 0; (ptran = pMakeNthTranslator(n, image, 0)); n++)
			AddTranslatorToList(ptran, path, image, false);
		
		return B_NO_ERROR;
		
	} else {
		// If the translator add-on is in the R4.0 / R4.5 format		
		translator_data trandata;
		
		//	find all the symbols
		err = get_image_symbol(image, "translatorName", B_SYMBOL_TYPE_DATA,
				(void **)&trandata.translatorName);
		if (!err && get_image_symbol(image, "translatorInfo",
				B_SYMBOL_TYPE_DATA, (void **)&trandata.translatorInfo))
			trandata.translatorInfo = NULL;
		long * vptr = NULL;
		if (!err)
			err = get_image_symbol(image, "translatorVersion",
					B_SYMBOL_TYPE_DATA, (void **)&vptr);
		if (!err && (vptr != NULL))
			trandata.translatorVersion = *vptr;
		if (!err && get_image_symbol(image, "inputFormats",
				B_SYMBOL_TYPE_DATA, (void **)&trandata.inputFormats))
			trandata.inputFormats = NULL;
		if (!err && get_image_symbol(image, "outputFormats",
				B_SYMBOL_TYPE_DATA, (void **)&trandata.outputFormats))
			trandata.outputFormats = NULL;
		if (!err)
			err = get_image_symbol(image, "Identify", B_SYMBOL_TYPE_TEXT,
					(void **)&trandata.Identify);
		if (!err)
			err = get_image_symbol(image, "Translate",
					B_SYMBOL_TYPE_TEXT, (void **)&trandata.Translate);
		if (!err && get_image_symbol(image, "MakeConfig",
				B_SYMBOL_TYPE_TEXT, (void **)&trandata.MakeConfig))
			trandata.MakeConfig = NULL;
		if (!err && get_image_symbol(image, "GetConfigMessage",
				B_SYMBOL_TYPE_TEXT, (void **)&trandata.GetConfigMessage))
			trandata.GetConfigMessage = NULL;

		// if add-on is not in the correct format, return with error
		if (err)
			return err;

		// add this translator to the list
		BR4xTranslator *pR4xTran = new BR4xTranslator(&trandata);
		AddTranslatorToList(pR4xTran, path, image, false);
			// do not call Acquire() on ptran because I want it to be
			// deleted the first time Release() is called on it.
			
		return B_NO_ERROR;
	}
}

// ---------------------------------------------------------------
// LoadDir
//
// Loads all of the translators in the directory path
//
// Preconditions: should only be called inside locked code
//
// Parameters: path, the directory of translators to load
//             loadErr, the return code
//             nLoaded, number of translators loaded
//
// Postconditions:
//
// Returns: B_FILE_NOT_FOUND, if the path is NULL or invalid
//          other errors if LoadTranslator failed
// ---------------------------------------------------------------
void
BTranslatorRoster::LoadDir(const char *path, int32 &loadErr, int32 &nLoaded)
{
	if (!path) {
		loadErr = B_FILE_NOT_FOUND;
		return;
	}

	loadErr = B_OK;
	DIR *dir = opendir(path);
	if (!dir) {
		loadErr = B_FILE_NOT_FOUND;
		return;
	}
	struct dirent *dent;
	struct stat stbuf;
	char cwd[PATH_MAX] = "";
	while (NULL != (dent = readdir(dir))) {
		strcpy(cwd, path);
		strcat(cwd, "/");
		strcat(cwd, dent->d_name);
		status_t err = stat(cwd, &stbuf);

		if (!err && S_ISREG(stbuf.st_mode) &&
			strcmp(dent->d_name, ".") && strcmp(dent->d_name, "..")) {
			
			err = LoadTranslator(cwd);
			if (err == B_OK)
				nLoaded++;
			else
				loadErr = err;
		}
	}
	closedir(dir);
}

// ---------------------------------------------------------------
// CheckFormats
//
// Function used to determine if hintType and hintMIME can be
// used to find the desired translation_format.
//
// Preconditions: Should only be called from inside locked code
//
// Parameters: inputFormats, the formats check against the hints
//             inputFormatsCount, number of items in inputFormats
//             hintType, the type this function is looking for
//             hintMIME, the MIME type this function is looking
//                       for
//             outFormat, NULL if no suitable translation_format
//                        from inputFormats was found, not NULL
//                        if a translation_format matching the
//                        hints was found
//           
//
// Postconditions:
//
// Returns: true, if a determination could be made
//          false, if indentification must be done
// ---------------------------------------------------------------
bool
BTranslatorRoster::CheckFormats(const translation_format *inputFormats,
	int32 inputFormatsCount, uint32 hintType, const char *hintMIME,
	const translation_format **outFormat)
{
	if (!inputFormats || inputFormatsCount <= 0 || !outFormat)
		return false;

	*outFormat = NULL;

	// return false if we can't use hints for this module
	if (!hintType && !hintMIME)
		return false;

	// check for the length of the MIME string, since it may be just a prefix
	// so we use strncmp().
	int mlen = 0;
	if (hintMIME)
		mlen = strlen(hintMIME);

	// scan for suitable format
	const translation_format *fmt = inputFormats;
	for (int32 i = 0; i < inputFormatsCount && fmt->type; i++, fmt++) {
		if ((fmt->type == hintType) ||
			(hintMIME && mlen && !strncmp(fmt->MIME, hintMIME, mlen))) {
			*outFormat = fmt;
			return true;
		}
	}
	// the module did export formats, but none matched.
	// we return true (uses formats) but set outFormat to NULL
	return true;
}

// ---------------------------------------------------------------
// AddTranslatorToList
//
// Adds a BTranslator based object to the list of translators
// stored by the BTranslatorRoster.
//
// Preconditions: Should only be called inside locked code
//
// Parameters: translator, the translator to add to this object
//
// Postconditions:
//
// Returns: B_BAD_VALUE, if translator is NULL
//          B_OK, if not
// ---------------------------------------------------------------
status_t
BTranslatorRoster::AddTranslatorToList(BTranslator *translator)
{
	return AddTranslatorToList(translator, "", -1, true);
		// add translator to list with no add-on image to unload,
		// and call Acquire() on it
}

// ---------------------------------------------------------------
// AddTranslatorToList
//
// Adds a BTranslator based object to the list of translators
// stored by the BTranslatorRoster.
//
// Preconditions: Should only be called inside locked code
//
// Parameters: translator, the translator to add to this object
//             path, the path the translator was loaded from
//             image, the image the translator was loaded from
//             acquire, if true Acquire() is called on the
//                      translator
//
// Postconditions:
//
// Returns: B_BAD_VALUE, if translator is NULL
//          B_OK, if not
// ---------------------------------------------------------------
status_t
BTranslatorRoster::AddTranslatorToList(BTranslator *translator,
	const char *path, image_id image, bool acquire)
{
	if (!translator || !path)
		return B_BAD_VALUE;

	translator_node *pTranNode = new translator_node;
	
	if (acquire)
		pTranNode->translator = translator->Acquire();
	else
		pTranNode->translator = translator;
		
	if (fpTranslators)
		pTranNode->id = fpTranslators->id + 1;
	else
		pTranNode->id = 1;
	
	pTranNode->path = new char[strlen(path) + 1];
	strcpy(pTranNode->path, path);
	pTranNode->image = image;
	pTranNode->next = fpTranslators;
	fpTranslators = pTranNode;
	
	return B_OK;
}

// ---------------------------------------------------------------
// ReservedTranslatorRoster1
//
// It does nothing! :) Its here only for past/future binary
// compatibility.
//
// Preconditions:
//
// Parameters:
//
// Postconditions:
//
// Returns:
// ---------------------------------------------------------------
void
BTranslatorRoster::ReservedTranslatorRoster1()
{
}

// ---------------------------------------------------------------
// ReservedTranslatorRoster2
//
// It does nothing! :) Its here only for past/future binary
// compatibility.
//
// Preconditions:
//
// Parameters:
//
// Postconditions:
//
// Returns:
// ---------------------------------------------------------------
void
BTranslatorRoster::ReservedTranslatorRoster2()
{
}

// ---------------------------------------------------------------
// ReservedTranslatorRoster3
//
// It does nothing! :) Its here only for past/future binary
// compatibility.
//
// Preconditions:
//
// Parameters:
//
// Postconditions:
//
// Returns:
// ---------------------------------------------------------------
void
BTranslatorRoster::ReservedTranslatorRoster3()
{
}

// ---------------------------------------------------------------
// ReservedTranslatorRoster4
//
// It does nothing! :) Its here only for past/future binary
// compatibility.
//
// Preconditions:
//
// Parameters:
//
// Postconditions:
//
// Returns:
// ---------------------------------------------------------------
void
BTranslatorRoster::ReservedTranslatorRoster4()
{
}

// ---------------------------------------------------------------
// ReservedTranslatorRoster5
//
// It does nothing! :) Its here only for past/future binary
// compatibility.
//
// Preconditions:
//
// Parameters:
//
// Postconditions:
//
// Returns:
// ---------------------------------------------------------------
void
BTranslatorRoster::ReservedTranslatorRoster5()
{
}

// ---------------------------------------------------------------
// ReservedTranslatorRoster6
//
// It does nothing! :) Its here only for past/future binary
// compatibility.
//
// Preconditions:
//
// Parameters:
//
// Postconditions:
//
// Returns:
// ---------------------------------------------------------------
void
BTranslatorRoster::ReservedTranslatorRoster6()
{
}

