/*
 * Copyright 2011, Oliver Tappe <zooey@hirschkaefer.de>
 * Distributed under the terms of the MIT License.
 */
#ifndef _PACKAGE__HPKG__PRIVATE__WRITER_IMPL_BASE_H_
#define _PACKAGE__HPKG__PRIVATE__WRITER_IMPL_BASE_H_


#include <util/DoublyLinkedList.h>

#include <package/hpkg/HPKGDefsPrivate.h>

#include <package/hpkg/DataOutput.h>
#include <package/hpkg/Strings.h>
#include <package/hpkg/ZlibCompressor.h>

#include <package/PackageInfo.h>


namespace BPackageKit {

namespace BHPKG {


class BDataReader;
class BErrorOutput;


namespace BPrivate {


struct hpkg_header;

class WriterImplBase {
public:
								WriterImplBase(BErrorOutput* errorOutput);
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


			struct AbstractDataWriter {
				AbstractDataWriter();
				virtual ~AbstractDataWriter();

				uint64 BytesWritten() const;

				virtual status_t WriteDataNoThrow(const void* buffer,
					size_t size) = 0;

				void WriteDataThrows(const void* buffer, size_t size);

			protected:
				uint64	fBytesWritten;
			};


			struct FDDataWriter : AbstractDataWriter {
				FDDataWriter(int fd, off_t offset, BErrorOutput* errorOutput);

				virtual status_t WriteDataNoThrow(const void* buffer,
					size_t size);

				off_t Offset() const;

			private:
				int				fFD;
				off_t			fOffset;
				BErrorOutput*	fErrorOutput;
			};


			struct ZlibDataWriter : AbstractDataWriter, private BDataOutput {
				ZlibDataWriter(AbstractDataWriter* dataWriter);

				void Init();

				void Finish();

				virtual status_t WriteDataNoThrow(const void* buffer,
					size_t size);

			private:
				// BDataOutput
				virtual status_t WriteData(const void* buffer, size_t size);

			private:
				AbstractDataWriter*	fDataWriter;
				ZlibCompressor		fCompressor;
			};


			typedef DoublyLinkedList<PackageAttribute> PackageAttributeList;

protected:
			status_t			Init(const char* fileName, const char* type);

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
	inline	void				WriteString(const char* string);

	template<typename Type>
	inline	void				Write(const Type& value);

			void				WriteBuffer(const void* buffer, size_t size,
									off_t offset);

	inline	int					FD() const;

	inline	const PackageAttributeList&	PackageAttributes() const;
	inline	PackageAttributeList&	PackageAttributes();

	inline	const StringCache&	PackageStringCache() const;
	inline	StringCache&		PackageStringCache();

	inline	AbstractDataWriter*	DataWriter() const;
	inline	void				SetDataWriter(AbstractDataWriter* dataWriter);

	inline	void				SetFinished(bool finished);

private:
	static const BHPKGAttributeID kDefaultVersionAttributeID
		= B_HPKG_ATTRIBUTE_ID_PACKAGE_VERSION_MAJOR;

private:
			void				_WritePackageAttributes(
									const PackageAttributeList& attributes);

private:
			BErrorOutput*		fErrorOutput;
			const char*			fFileName;
			int					fFD;
			bool				fFinished;

			AbstractDataWriter*	fDataWriter;

			StringCache			fPackageStringCache;
			PackageAttributeList	fPackageAttributes;
};


inline uint64
WriterImplBase::AbstractDataWriter::BytesWritten() const
{
	return fBytesWritten;
}


inline void
WriterImplBase::AbstractDataWriter::WriteDataThrows(const void* buffer,
	size_t size)
{
	status_t error = WriteDataNoThrow(buffer, size);
	if (error != B_OK)
		throw status_t(error);
}

inline off_t
WriterImplBase::FDDataWriter::Offset() const
{
	return fOffset;
}


template<typename Type>
inline void
WriterImplBase::Write(const Type& value)
{
	fDataWriter->WriteDataThrows(&value, sizeof(Type));
}


inline void
WriterImplBase::WriteString(const char* string)
{
	fDataWriter->WriteDataThrows(string, strlen(string) + 1);
}


inline int
WriterImplBase::FD() const
{
	return fFD;
}


inline WriterImplBase::AbstractDataWriter*
WriterImplBase::DataWriter() const
{
	return fDataWriter;
}


inline void
WriterImplBase::SetDataWriter(AbstractDataWriter* dataWriter)
{
	fDataWriter = dataWriter;
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


}	// namespace BPrivate

}	// namespace BHPKG

}	// namespace BPackageKit


#endif	// _PACKAGE__HPKG__PRIVATE__WRITER_IMPL_BASE_H_
