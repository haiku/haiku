// FlatMessageIO.cpp
// e.moon 6jul99

#include "FlatMessageIO.h"
//#include "xml_export_utils.h"

#include "ExportContext.h"

#include <Debug.h>

// base64 encoding tools
#include <E-mail.h>

#include <cstdlib>
#include <cstring>

__USE_CORTEX_NAMESPACE

// -------------------------------------------------------- //
// *** constants
// -------------------------------------------------------- //

const char* const FlatMessageIO::s_element 				= "flat-BMessage";

// -------------------------------------------------------- //
// *** ctor/dtor/accessor
// -------------------------------------------------------- //

FlatMessageIO::~FlatMessageIO() {
	if(m_ownMessage && m_message)
		delete m_message;
}
FlatMessageIO::FlatMessageIO() :
	m_ownMessage(true),
	m_message(0) {}
FlatMessageIO::FlatMessageIO(const BMessage* message) :
	m_ownMessage(false),
	m_message(const_cast<BMessage*>(message)) {}

void FlatMessageIO::setMessage(BMessage* message) {
	if(m_ownMessage && m_message)
		delete m_message;
	m_ownMessage = false;
	m_message = message;
}

// -------------------------------------------------------- //
// *** static setup method
// -------------------------------------------------------- //

// call this method to install hooks for the tags needed by
// FlatMessageIO into the given document type
/*static*/
void FlatMessageIO::AddTo(XML::DocumentType* pDocType) {
	pDocType->addMapping(new Mapping<FlatMessageIO>(s_element));
}

// -------------------------------------------------------- //
// *** IPersistent impl.
// -------------------------------------------------------- //


void FlatMessageIO::xmlExportBegin(
	ExportContext& context) const {
	
	context.beginElement(s_element);
}

void FlatMessageIO::xmlExportAttributes(
	ExportContext& context) const {
	
	context.writeAttr("encoding", "base64");
}
	
void FlatMessageIO::xmlExportContent(
	ExportContext& context) const {

	context.beginContent();

	// convert message to base64
	ASSERT(m_message);
	ssize_t flatSize = m_message->FlattenedSize();
	ASSERT(flatSize);
	char* flatData = new char[flatSize];
	status_t err = m_message->Flatten(flatData, flatSize);
	ASSERT(err == B_OK);
	
	// make plenty of room for encoded content (encode_base64 adds newlines)
	ssize_t base64Size = ((flatSize * 3) / 2);
	char* base64Data = new char[base64Size];
	ssize_t base64Used = encode_base64(base64Data, flatData, flatSize);
	base64Data[base64Used] = '\0';
	
	// write the data

	const char* pos = base64Data;
	while(*pos) {
		ssize_t chunk = 0;
		const char* nextBreak = strchr(pos, '\n');
		if(!nextBreak)
			chunk = strlen(pos);
		else
			chunk = nextBreak - pos;

		context.writeString(context.indentString());
		context.writeString(pos, chunk);
		context.writeString("\n");
		
		pos += chunk;
		if(*pos == '\n')
			++pos;
	}
	
	// clean up
	delete [] flatData;
	delete [] base64Data;
}
	
void FlatMessageIO::xmlExportEnd(
	ExportContext& context) const {
	context.endElement();
}

// -------------------------------------------------------- //
	
void FlatMessageIO::xmlImportBegin(
	ImportContext&		context) {

	m_base64Data = "";
}

void FlatMessageIO::xmlImportAttribute(
	const char*					key,
	const char*					value,
	ImportContext&		context) {
	if(!strcmp(key, "encoding")) {
		if(strcmp(value, "base64") != 0)
			context.reportError("Unexpected value of 'encoding'.");
	}
}
	
void FlatMessageIO::xmlImportContent(
	const char*					data,
	uint32						length,
	ImportContext&		context) {
	m_base64Data.Append(data, length);
}
	
void FlatMessageIO::xmlImportChild(
	IPersistent*			child,
	ImportContext&		context) {
	delete child;
	context.reportError("Unexpected child object.");
}
			
void FlatMessageIO::xmlImportComplete(
	ImportContext&		context) {
	
	if(m_ownMessage && m_message)
		delete m_message;	
	
	// decode the message
	ssize_t decodedSize = m_base64Data.Length();
	char* decodedData = new char[decodedSize];
	decodedSize = decode_base64(
		decodedData, const_cast<char*>(m_base64Data.String()),
		m_base64Data.Length(), false);
		
	m_message = new BMessage();
	m_ownMessage = true;
	
	status_t err = m_message->Unflatten(decodedData);
	if(err < B_OK) {
		// decode failed; report error & clean up
		BString error = "Unflatten(): ";
		error << strerror(err);
		context.reportError(error.String());

		delete m_message;
		m_message = 0;
	}
	
	m_base64Data = "";
	delete [] decodedData;
}

// END -- FlatMessageIO.cpp --
