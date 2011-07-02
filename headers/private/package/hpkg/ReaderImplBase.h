/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2011, Oliver Tappe <zooey@hirschkaefer.de>
 * Distributed under the terms of the MIT License.
 */
#ifndef _PACKAGE__HPKG__PRIVATE__READER_IMPL_BASE_H_
#define _PACKAGE__HPKG__PRIVATE__READER_IMPL_BASE_H_


#include <SupportDefs.h>

#include <util/SinglyLinkedList.h>

#include <package/hpkg/PackageAttributeValue.h>
#include <package/hpkg/PackageContentHandler.h>
#include <package/hpkg/PackageInfoAttributeValue.h>


namespace BPackageKit {

namespace BHPKG {


class BErrorOutput;


namespace BPrivate {


class ReaderImplBase {
protected:
								ReaderImplBase(
									BErrorOutput* errorOutput);
	virtual						~ReaderImplBase();

	virtual	status_t			Init(int fd, bool keepFD);

			int					FD() const;

			BErrorOutput*		ErrorOutput() const;

protected:
			struct AttributeHandlerContext {
				BErrorOutput*	errorOutput;
				union {
					BPackageContentHandler*			packageContentHandler;
					BLowLevelPackageContentHandler*	lowLevelHandler;
				};
				bool			hasLowLevelHandler;

				uint64			heapOffset;
				uint64			heapSize;

				AttributeHandlerContext(BErrorOutput* errorOutput,
					BPackageContentHandler* packageContentHandler);

				AttributeHandlerContext(BErrorOutput* errorOutput,
					BLowLevelPackageContentHandler* lowLevelHandler);

				void ErrorOccurred();
			};


			typedef BPackageAttributeValue AttributeValue;

			struct AttributeHandler
				: SinglyLinkedListLinkImpl<AttributeHandler> {
				virtual ~AttributeHandler();

				void SetLevel(int level);
				virtual status_t HandleAttribute(
					AttributeHandlerContext* context, uint8 id,
					const AttributeValue& value, AttributeHandler** _handler);

				virtual status_t Delete(AttributeHandlerContext* context);

			protected:
				int	fLevel;
			};


			struct IgnoreAttributeHandler : AttributeHandler {
			};


			struct PackageVersionAttributeHandler : AttributeHandler {
				PackageVersionAttributeHandler(
					BPackageInfoAttributeValue& packageInfoValue,
					BPackageVersionData& versionData, bool notify);

				virtual status_t HandleAttribute(
					AttributeHandlerContext* context, uint8 id,
					const AttributeValue& value, AttributeHandler** _handler);

				virtual status_t Delete(AttributeHandlerContext* context);

			private:
				BPackageInfoAttributeValue&	fPackageInfoValue;
				BPackageVersionData&		fPackageVersionData;
				bool						fNotify;
			};


			struct PackageResolvableAttributeHandler : AttributeHandler {
				PackageResolvableAttributeHandler(
					BPackageInfoAttributeValue& packageInfoValue);

				virtual status_t HandleAttribute(
					AttributeHandlerContext* context, uint8 id,
					const AttributeValue& value, AttributeHandler** _handler);

				virtual status_t Delete(AttributeHandlerContext* context);

			private:
				BPackageInfoAttributeValue&	fPackageInfoValue;
			};


			struct PackageResolvableExpressionAttributeHandler
				: AttributeHandler {
				PackageResolvableExpressionAttributeHandler(
					BPackageInfoAttributeValue& packageInfoValue);

				virtual status_t HandleAttribute(
					AttributeHandlerContext* context, uint8 id,
					const AttributeValue& value, AttributeHandler** _handler);

				virtual status_t Delete(AttributeHandlerContext* context);

			private:
				BPackageInfoAttributeValue&	fPackageInfoValue;
			};


			struct PackageAttributeHandler : AttributeHandler {
				virtual status_t HandleAttribute(
					AttributeHandlerContext* context, uint8 id,
					const AttributeValue& value, AttributeHandler** _handler);

			private:
				BPackageInfoAttributeValue	fPackageInfoValue;
			};


			struct LowLevelAttributeHandler : AttributeHandler {
				LowLevelAttributeHandler();
				LowLevelAttributeHandler(uint8 id,
					const BPackageAttributeValue& value, void* parentToken,
					void* token);

				virtual status_t HandleAttribute(
					AttributeHandlerContext* context, uint8 id,
					const AttributeValue& value, AttributeHandler** _handler);
				virtual status_t Delete(AttributeHandlerContext* context);

			private:
				void*			fParentToken;
				void*			fToken;
				uint8			fID;
				AttributeValue	fValue;
			};


			struct SectionInfo {
				uint32			compression;
				uint32			compressedLength;
				uint32			uncompressedLength;
				uint8*			data;
				uint64			offset;
				uint64			currentOffset;
				uint64			stringsLength;
				uint64			stringsCount;
				char**			strings;
				const char*		name;

				SectionInfo(const char* _name)
					:
					data(NULL),
					strings(NULL),
					name(_name)
				{
				}

				~SectionInfo()
				{
					delete[] strings;
					delete[] data;
				}
			};

			typedef SinglyLinkedList<AttributeHandler> AttributeHandlerList;

protected:
			const char*			CheckCompression(
									const SectionInfo& section) const;

			status_t			ParseStrings();

			status_t			ParsePackageAttributesSection(
									AttributeHandlerContext* context,
									AttributeHandler* rootAttributeHandler);
			status_t			ParseAttributeTree(
									AttributeHandlerContext* context);

	virtual	status_t			ReadAttributeValue(uint8 type, uint8 encoding,
									AttributeValue& _value);

			status_t			ReadUnsignedLEB128(uint64& _value);

			status_t			ReadBuffer(off_t offset, void* buffer,
									size_t size);
			status_t			ReadCompressedBuffer(
									const SectionInfo& section);

	inline	AttributeHandler*	CurrentAttributeHandler() const;
	inline	void				PushAttributeHandler(
									AttributeHandler* handler);
	inline	AttributeHandler*	PopAttributeHandler();
	inline	void				ClearAttributeHandlerStack();

	inline	SectionInfo*		CurrentSection();
	inline	void				SetCurrentSection(SectionInfo* section);

protected:
			SectionInfo			fPackageAttributesSection;

private:
	template<typename Type>
	inline	status_t			_Read(Type& _value);

			status_t			_ReadSectionBuffer(void* buffer, size_t size);

			status_t			_ReadAttribute(uint8& _id,
									AttributeValue& _value,
									bool* _hasChildren = NULL,
									uint64* _tag = NULL);

			status_t			_ReadString(const char*& _string,
									size_t* _stringLength = NULL);

private:
			BErrorOutput*		fErrorOutput;
			int					fFD;
			bool				fOwnsFD;

			SectionInfo*		fCurrentSection;

			AttributeHandlerList fAttributeHandlerStack;

			uint8*				fScratchBuffer;
			size_t				fScratchBufferSize;
};


inline int
ReaderImplBase::FD() const
{
	return fFD;
}


inline BErrorOutput*
ReaderImplBase::ErrorOutput() const
{
	return fErrorOutput;
}


ReaderImplBase::SectionInfo*
ReaderImplBase::CurrentSection()
{
	return fCurrentSection;
}


void
ReaderImplBase::SetCurrentSection(SectionInfo* section)
{
	fCurrentSection = section;
}


template<typename Type>
status_t
ReaderImplBase::_Read(Type& _value)
{
	return _ReadSectionBuffer(&_value, sizeof(Type));
}


inline ReaderImplBase::AttributeHandler*
ReaderImplBase::CurrentAttributeHandler() const
{
	return fAttributeHandlerStack.Head();
}


inline void
ReaderImplBase::PushAttributeHandler(AttributeHandler* handler)
{
	fAttributeHandlerStack.Add(handler);
}


inline ReaderImplBase::AttributeHandler*
ReaderImplBase::PopAttributeHandler()
{
	return fAttributeHandlerStack.RemoveHead();
}


inline void
ReaderImplBase::ClearAttributeHandlerStack()
{
	fAttributeHandlerStack.MakeEmpty();
}


}	// namespace BPrivate

}	// namespace BHPKG

}	// namespace BPackageKit


#endif	// _PACKAGE__HPKG__PRIVATE__READER_IMPL_BASE_H_
