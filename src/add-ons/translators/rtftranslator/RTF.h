/*
 * Copyright 2004-2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef RTF_H
#define RTF_H


#include "Stack.h"

#include <List.h>
#include <String.h>
#include <GraphicsDefs.h>

class BDataIO;
class RTFCommand;


static const size_t kRTFCommandLength = 32;


enum rtf_destination {
	RTF_TEXT,
	RTF_COMMENT,
	RTF_OTHER,
	
	RTF_ALL_DESTINATIONS = 255
};

class RTFElement {
	public:
		RTFElement();
		virtual ~RTFElement();

		status_t AddElement(RTFElement *element);
		uint32 CountElements() const;
		RTFElement *ElementAt(uint32 index) const;
		RTFCommand *FindDefinition(const char *name, int32 index = 0) const;
		RTFElement *FindGroup(const char *name) const;
		const char *GroupName() const;
		RTFElement *Parent() const;

		virtual void Parse(char first, BDataIO &stream, char &last) throw (status_t);
		virtual void PrintToStream();

		void DetermineDestination();
		rtf_destination Destination() const;

	protected:
		static char ReadChar(BDataIO &stream, bool endOfFileAllowed = false) throw (status_t);
		static int32 ParseInteger(char first, BDataIO &stream, char &last) throw (status_t);

	private:
		RTFElement	*fParent;
		BList		fElements;
		rtf_destination	fDestination;
};

class RTFHeader : public RTFElement {
	public:
		RTFHeader();
		virtual ~RTFHeader();

		int32 Version() const;
		const char *Charset() const;

		rgb_color Color(int32 index);

		static status_t Identify(BDataIO &stream);
		static status_t Parse(BDataIO &stream, RTFHeader &header, bool identified = false);

	protected:
		virtual void Parse(char first, BDataIO &stream, char &last) throw (status_t);

	private:
		int32		fVersion;
		
};

class RTFText : public RTFElement {
	public:
		RTFText();
		virtual ~RTFText();

		status_t SetText(const char *text);
		const char *Text() const;
		uint32 TextLength() const;

	protected:
		virtual void Parse(char first, BDataIO &stream, char &last) throw (status_t);

	private:
		BString		fText;
};

class RTFCommand : public RTFElement {
	public:
		RTFCommand();
		virtual ~RTFCommand();

		status_t SetName(const char *name);
		const char *Name();

		void UnsetOption();
		void SetOption(int32 option);
		bool HasOption() const;
		int32 Option() const;

	protected:
		virtual void Parse(char first, BDataIO &stream, char &last) throw (status_t);

	private:
		BString		fName;
		bool		fHasOption;
		int32		fOption;
};

//---------------------------------

class RTFIterator {
	public:
		RTFIterator(RTFElement &start, rtf_destination destination = RTF_ALL_DESTINATIONS);

		void SetTo(RTFElement &start, rtf_destination destination = RTF_ALL_DESTINATIONS);
		void Rewind();

		bool HasNext() const;
		RTFElement *Next();

	private:
		RTFElement			*fStart;
		Stack<RTFElement *>	fStack;
		rtf_destination		fDestination;
};

#endif	/* RTF_H */
