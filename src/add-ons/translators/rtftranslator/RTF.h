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
#include <BufferIO.h>


namespace RTF {

class Group;
class Header;
class Command;

static const size_t kCommandLength = 32;

enum group_destination {
	TEXT_DESTINATION,
	COMMENT_DESTINATION,
	OTHER_DESTINATION,

	ALL_DESTINATIONS = 255
};


class Parser {
	public:
		Parser(BPositionIO &stream);

		status_t Identify();
		status_t Parse(RTF::Header &header);

	private:
		BBufferIO	fStream;
		bool		fIdentified;
};


class Element {
	public:
		Element();
		virtual ~Element();

		void SetParent(Group *parent);
		Group *Parent() const;

		virtual bool IsDefinitionDelimiter();
		virtual void Parse(char first, BDataIO &stream, char &last) throw (status_t) = 0;
		virtual void PrintToStream(int32 level = 0);

	private:
		Group	*fParent;
};


class Group : public Element {
	public:
		Group();
		virtual ~Group();

		status_t AddElement(RTF::Element *element);
		uint32 CountElements() const;
		Element *ElementAt(uint32 index) const;

		Element *FindDefinitionStart(int32 index, int32 *_startIndex = NULL) const;
		Command *FindDefinition(const char *name, int32 index = 0) const;
		Group *FindGroup(const char *name) const;

		const char *Name() const;

		void DetermineDestination();
		group_destination Destination() const;

		virtual void Parse(char first, BDataIO &stream, char &last) throw (status_t);

	protected:
		BList				fElements;
		group_destination	fDestination;
};

class Header : public Group {
	public:
		Header();
		virtual ~Header();

		int32 Version() const;
		const char *Charset() const;

		rgb_color Color(int32 index);

		virtual void Parse(char first, BDataIO &stream, char &last) throw (status_t);

	private:
		int32				fVersion;
		
};

class Text : public Element {
	public:
		Text();
		virtual ~Text();

		status_t SetTo(const char *text);
		const char *String() const;
		uint32 Length() const;

		virtual bool IsDefinitionDelimiter();
		virtual void Parse(char first, BDataIO &stream, char &last) throw (status_t);

	private:
		BString				fText;
};

class Command : public Element {
	public:
		Command();
		virtual ~Command();

		status_t SetName(const char *name);
		const char *Name();

		void UnsetOption();
		void SetOption(int32 option);
		bool HasOption() const;
		int32 Option() const;

		virtual void Parse(char first, BDataIO &stream, char &last) throw (status_t);

	private:
		BString				fName;
		bool				fHasOption;
		int32				fOption;
};

//---------------------------------

class Iterator {
	public:
		Iterator(Element &start, group_destination destination = ALL_DESTINATIONS);

		void SetTo(Element &start, group_destination destination = ALL_DESTINATIONS);
		void Rewind();

		bool HasNext() const;
		Element *Next();

	private:
		Element				*fStart;
		Stack<Element *>	fStack;
		group_destination	fDestination;
};

class Worker {
	public:
		Worker(RTF::Header &start);
		virtual ~Worker();

		void Work() throw (status_t);

	protected:
		virtual void Group(RTF::Group *group);
		virtual void GroupEnd(RTF::Group *group);
		virtual void Command(RTF::Command *command);
		virtual void Text(RTF::Text *text);

		RTF::Header &Start();
		void Skip();
		void Abort(status_t status);

	private:
		void Dispatch(RTF::Element *element);

		RTF::Header	&fStart;
		bool		fSkip;
};

}	// namespace RTF

#endif	/* RTF_H */
