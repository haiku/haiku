// ResourceItem.h

#ifndef RESOURCE_ITEM_H
#define RESOURCE_ITEM_H

#include <String.h>
#include <SupportDefs.h>

class BPositionIO;

class ResourceItem {
public:
	typedef int32 roff_t;

public:
								ResourceItem();
	virtual						~ResourceItem();

			void				SetLocation(roff_t offset, roff_t size);
			void				SetIdentity(type_code type, int32 id,
											const char* name);

			void				SetOffset(roff_t offset);
			roff_t				GetOffset() const;

			void				SetSize(roff_t size);
			roff_t				GetSize() const;

			void				SetType(type_code type);
			type_code			GetType() const;

			void				SetID(int32 id);
			int32				GetID() const;

			void				SetName(const char* name);
			const char*			GetName() const;

			void				SetData(const void* data, roff_t size = -1);
			void				UnsetData();
			void*				AllocData(roff_t size = -1);
			void*				GetData() const;

			status_t			LoadData(BPositionIO& file,
										 roff_t position = -1,
										 roff_t size = -1);
			status_t			WriteData(BPositionIO& file) const;

			void				PrintToStream();

private:
			roff_t				fOffset;
			roff_t				fSize;
			type_code			fType;
			int32				fID;
			BString				fName;
			void*				fData;
};

#endif	// RESOURCE_ITEM_H
