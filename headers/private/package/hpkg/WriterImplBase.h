/*
 * Copyright 2011, Oliver Tappe <zooey@hirschkaefer.de>
 * Distributed under the terms of the MIT License.
 */
#ifndef _PACKAGE__HPKG__PRIVATE__WRITER_IMPL_BASE_H_
#define _PACKAGE__HPKG__PRIVATE__WRITER_IMPL_BASE_H_


#include <util/DoublyLinkedList.h>

#include <package/hpkg/HPKGDefsPrivate.h>

#include <package/hpkg/PackageWriter.h>
#include <package/hpkg/Strings.h>

#include <package/PackageInfo.h>

#include <package/hpkg/PackageFileHeapWriter.h>


class BCompressionAlgorithm;
class BCompressionParameters;
class BDecompressionParameters;


namespace BPackageKit {

namespace BHPKG {


class BDataReader;
class BErrorOutput;


namespace BPrivate {


struct hpkg_header;


class WriterImplBase {
public:
								WriterImplBase(const char* fileType,
									BErrorOutput* errorOutput);
								~WriterImplBase();

protected:
			struct AttributeValue {
				union {
					int64			signedInt;
					uint64			unsignedInt;
					CachedString*	string;
					struct {
						uint64		size;
						union {
							uint64	offset;
							uint8	raw[B_HPKG_MAX_INLINE_DATA_SIZE];
						};
					} data;
				};
				uint8		type;
				int8		encoding;

				AttributeValue();
				~AttributeValue();

				void SetTo(int8 value);
				void SetTo(uint8 value);
				void SetTo(int16 value);
				void SetTo(uint16 value);
				void SetTo(int32 value);
				void SetTo(uint32 value);
				void SetTo(int64 value);
				void SetTo(uint64 value);
				void SetTo(CachedString* value);
				void SetToData(uint64 size, uint64 offset);
				void SetToData(uint64 size, const void* rawData);
				uint8 ApplicableEncoding() const;

			private:
				static uint8 _ApplicableIntEncoding(uint64 value);
			};


			struct PackageAttribute :
					public DoublyLinkedListLinkImpl<PackageAttribute>,
					public AttributeValue {
				BHPKGAttributeID 					id;
				DoublyLinkedList<PackageAttribute>	children;

				PackageAttribute(BHPKGAttributeID id_, uint8 type,
					uint8 encoding);
				~PackageAttribute();

				void AddChild(PackageAttribute* child);

			private:
				void _DeleteChildren();
			};

			typedef DoublyLinkedList<PackageAttribute> PackageAttributeList;

protected:
			status_t			Init(BPositionIO* file, bool keepFile,
									const char* fileName,
									const BPackageWriterParameters& parameters);
			status_t			InitHeapReader(size_t headerSize);

			void				SetCompression(uint32 compression);

			void				RegisterPackageInfo(
									PackageAttributeList& attributeList,
									const BPackageInfo& packageInfo);
			void				RegisterPackageVersion(
									PackageAttributeList& attributeList,
									const BPackageVersion& version,
									BHPKGAttributeID attributeID
										= kDefaultVersionAttributeID);
			void				RegisterPackageResolvableExpressionList(
									PackageAttributeList& attributeList,
									const BObjectList<
										BPackageResolvableExpression>& list,
									uint8 id);

			PackageAttribute*	AddStringAttribute(BHPKGAttributeID id,
									const BString& value,
									DoublyLinkedList<PackageAttribute>& list);

			int32				WriteCachedStrings(const StringCache& cache,
									uint32 minUsageCount);

			int32				WritePackageAttributes(
									const PackageAttributeList& attributes,
									uint32& _stringsLengthUncompressed);
			void				WritePackageVersion(
									const BPackageVersion& version);
			void				WritePackageResolvableExpressionList(
									const BObjectList<
										BPackageResolvableExpression>& list,
									uint8 id);

			void				WriteAttributeValue(const AttributeValue& value,
									uint8 encoding);
			void				WriteUnsignedLEB128(uint64 value);

	template<typename Type>
	inline	void				Write(const Type& value);
	inline	void				WriteString(const char* string);
	inline	void				WriteBuffer(const void* data, size_t size);
									// appends data to the heap

			void				RawWriteBuffer(const void* buffer, size_t size,
									off_t offset);
									// writes to the file directly

	inline	BPositionIO*		File() const;
	inline	uint32				Flags() const;
	inline	const BPackageWriterParameters& Parameters() const;

	inline	const PackageAttributeList&	PackageAttributes() const;
	inline	PackageAttributeList&	PackageAttributes();

	inline	const StringCache&	PackageStringCache() const;
	inline	StringCache&		PackageStringCache();

	inline	void				SetFinished(bool finished);

protected:
			PackageFileHeapWriter* fHeapWriter;
			BCompressionAlgorithm* fCompressionAlgorithm;
			BCompressionParameters* fCompressionParameters;
			BCompressionAlgorithm* fDecompressionAlgorithm;
			BDecompressionParameters* fDecompressionParameters;

private:
	static const BHPKGAttributeID kDefaultVersionAttributeID
		= B_HPKG_ATTRIBUTE_ID_PACKAGE_VERSION_MAJOR;

private:
	inline	PackageAttribute*	_AddStringAttributeIfNotEmpty(
									BHPKGAttributeID id, const BString& value,
									DoublyLinkedList<PackageAttribute>& list);
			void				_AddStringAttributeList(BHPKGAttributeID id,
									const BStringList& value,
									DoublyLinkedList<PackageAttribute>& list);
			void				_WritePackageAttributes(
									const PackageAttributeList& attributes);

private:
			const char*			fFileType;
			BErrorOutput*		fErrorOutput;
			const char*			fFileName;
			BPackageWriterParameters fParameters;
			BPositionIO*		fFile;
			bool				fOwnsFile;
			bool				fFinished;

			StringCache			fPackageStringCache;
			PackageAttributeList	fPackageAttributes;
};


template<typename Type>
inline void
WriterImplBase::Write(const Type& value)
{
	WriteBuffer(&value, sizeof(Type));
}


inline void
WriterImplBase::WriteString(const char* string)
{
	WriteBuffer(string, strlen(string) + 1);
}


inline void
WriterImplBase::WriteBuffer(const void* data, size_t size)
{
	fHeapWriter->AddDataThrows(data, size);
}


inline BPositionIO*
WriterImplBase::File() const
{
	return fFile;
}


inline uint32
WriterImplBase::Flags() const
{
	return fParameters.Flags();
}


inline const BPackageWriterParameters&
WriterImplBase::Parameters() const
{
	return fParameters;
}


inline const WriterImplBase::PackageAttributeList&
WriterImplBase::PackageAttributes() const
{
	return fPackageAttributes;
}


inline WriterImplBase::PackageAttributeList&
WriterImplBase::PackageAttributes()
{
	return fPackageAttributes;
}


inline const StringCache&
WriterImplBase::PackageStringCache() const
{
	return fPackageStringCache;
}


inline StringCache&
WriterImplBase::PackageStringCache()
{
	return fPackageStringCache;
}


inline void
WriterImplBase::SetFinished(bool finished)
{
	fFinished = finished;
}


inline WriterImplBase::PackageAttribute*
WriterImplBase::_AddStringAttributeIfNotEmpty(BHPKGAttributeID id,
	const BString& value, DoublyLinkedList<PackageAttribute>& list)
{
	if (value.IsEmpty())
		return NULL;
	return AddStringAttribute(id, value, list);
}


}	// namespace BPrivate

}	// namespace BHPKG

}	// namespace BPackageKit


#endif	// _PACKAGE__HPKG__PRIVATE__WRITER_IMPL_BASE_H_
