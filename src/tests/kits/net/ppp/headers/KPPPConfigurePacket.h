#ifndef _K_PPP_CONFIGURE_PACKET__H
#define _K_PPP_CONFIGURE_PACKET__H

#include <mbuf.h>


typedef struct configure_item {
	uint8 type;
	uint8 length;
	int8 data[0];
		// the data follows this structure
} configure_item;


class PPPConfigurePacket {
	private:
		// copies are not allowed!
		PPPConfigurePacket(const PPPConfigurePacket& copy);
		PPPConfigurePacket& operator= (const PPPConfigurePacket& copy);

	public:
		PPPConfigurePacket(uint8 code);
		PPPConfigurePacket(mbuf *data);
		~PPPConfigurePacket();
		
		bool SetCode(uint8 code);
		uint8 Code() const
			{ return fCode; }
		
		void SetID(uint8 id)
			{ fID = id; }
		uint8 ID() const
			{ return fID; }
		
		void AddItem(const configure_item *item);
		bool RemoveItem(const configure_item *item);
		int32 CountItems() const;
		configure_item *ItemAt(int32 index) const;
		
		mbuf *ToMbuf();
			// the user is responsible for freeing the mbuf

	private:
		uint8 fCode, fID;
};


#endif
