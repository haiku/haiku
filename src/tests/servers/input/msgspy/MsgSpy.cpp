//-------------------------------------------------------------------------
// Handy InputFilter that dumps all Messages to a file.
//-------------------------------------------------------------------------
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <Debug.h>
#include <List.h>
#include <File.h>
#include <Message.h>
#include <String.h>
#include <OS.h>

#include <add-ons/input_server/InputServerFilter.h>

extern "C" _EXPORT BInputServerFilter* instantiate_input_filter();

class MsgSpy : public BInputServerFilter 
{
public:
	         MsgSpy();
	virtual ~MsgSpy();
	
    virtual status_t      InitCheck(void);
	virtual	filter_result Filter(BMessage *message, BList *outList);
private:
	const char* MapWhatToString(uint32 w);
	void        OutputMsgField(const char*  fieldName,
	                           const uint32 rawType,
                               int          rawCount,
                               const void*  rawData);

    status_t m_status;
    BFile*   m_outfile;
};

//-------------------------------------------------------------------------
// Create a new MsgSpy instance and return it to the caller.
//-------------------------------------------------------------------------
BInputServerFilter* instantiate_input_filter()
{
	return (new MsgSpy() );
}


//-------------------------------------------------------------------------
//-------------------------------------------------------------------------
MsgSpy::MsgSpy()
{
    // Create the output file and return its status.
    m_outfile = new BFile("/boot/home/MsgSpy.output", B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE);
    //m_status = m_outfile->InitCheck();
    m_status = B_OK;
}

//-------------------------------------------------------------------------
//-------------------------------------------------------------------------
MsgSpy::~MsgSpy()
{
    if (NULL != m_outfile)
    {
        // Close and destroy the output file.
        delete m_outfile;
    }
}

//-------------------------------------------------------------------------
//-------------------------------------------------------------------------
status_t MsgSpy::InitCheck(void)
{
	return m_status;
}

//-------------------------------------------------------------------------
//-------------------------------------------------------------------------
filter_result MsgSpy::Filter(BMessage *message, BList *outList)
{
    char*       field_name;
    uint32      field_type;
    int32       field_count;
    const void* field_data;
    ssize_t     field_data_bytes;
	char        msg_buffer  [1024];

	// Print out the message constant (what).
	sprintf(msg_buffer, "%s\n", MapWhatToString(message->what) );
    m_outfile->Write(msg_buffer, strlen(msg_buffer) );
    
    // Display each field in the message.
	sprintf(msg_buffer, "{\n");
    m_outfile->Write(msg_buffer, strlen(msg_buffer) );
	for (int32 i = 0;  B_OK == message->GetInfo(B_ANY_TYPE,
	                                            i,
	                                            &field_name,
	                                            &field_type,
	                                            &field_count);  i++)
	{
		message->FindData(field_name, field_type, &field_data, &field_data_bytes);
		OutputMsgField(field_name, field_type, field_count, field_data);
	}
	sprintf(msg_buffer, "}\n");
    m_outfile->Write(msg_buffer, strlen(msg_buffer) );
	    
	return (B_DISPATCH_MESSAGE);
}

//-------------------------------------------------------------------------
//-------------------------------------------------------------------------
const char* MsgSpy::MapWhatToString(uint32 w)
{
    const char* s;
	switch (w)
	{
		// Pointing device event messages.
		case B_MOUSE_DOWN:					s = "B_MOUSE_DOWN";					break;
		case B_MOUSE_UP:					s = "B_MOUSE_UP";					break;
		case B_MOUSE_MOVED:					s = "B_MOUSE_MOVED";				break;
		case B_MOUSE_WHEEL_CHANGED:				s = "B_MOUSE_WHEEL_CHANGED";			break;

		// Keyboard device event messages.
		case B_KEY_DOWN:					s = "B_KEY_DOWN";					break;
		case B_UNMAPPED_KEY_DOWN:			s = "B_UNMAPPED_KEY_DOWN";			break;
		case B_KEY_UP:						s = "B_KEY_UP";						break;
		case B_UNMAPPED_KEY_UP:				s = "B_UNMAPPED_KEY_UP";			break;
		case B_MODIFIERS_CHANGED:			s = "B_MODIFIERS_CHANGED";			break;
		
		case B_INPUT_METHOD_EVENT:			s = "B_INPUT_METHOD_EVENT";			break;
		default:							s = "UNKNOWN_MESSAGE";				break;
	}
	return s;
}

//-------------------------------------------------------------------------
//-------------------------------------------------------------------------
void MsgSpy::OutputMsgField(const char*  fieldName,
                            const uint32 rawType,
                            int          rawCount,
                            const void*  rawData)
{
	char        msg_buffer   [1024];
	char        value_buffer [256];
	BString     field_data;
    const char* field_type;
    const int   field_count  = rawCount;
    const char* separator;
    
	switch (rawType)
	{
		case B_CHAR_TYPE:
		{
			field_type = "B_CHAR_TYPE";
			field_data << "{ ";			
			for (const char* data_ptr = (const char*)rawData;
				rawCount > 0;
				rawCount--, data_ptr++)
			{
				separator = (1 < rawCount) ? ", " : " }";
				field_data << *data_ptr << separator;
			}
			break;
		}
			
		case B_INT8_TYPE:
		{
			field_type = "B_INT8_TYPE";
			field_data << "{ ";			
			for (const int8* data_ptr = (const int8*)rawData;
				rawCount > 0;
				rawCount--, data_ptr++)
			{
				separator = (1 < rawCount) ? ", " : " }";
				field_data << *data_ptr << separator;
			}
			break;
		}
		
		case B_INT16_TYPE:
		{
			field_type = "B_INT16_TYPE";
			field_data << "{ ";			
			for (const int16* data_ptr = (const int16*)rawData;
				rawCount > 0;
				rawCount--, data_ptr++)
			{
				separator = (1 < rawCount) ? ", " : " }";
				field_data << *data_ptr << separator;
			}
			break;
		}
		
		case B_INT32_TYPE:
		{
			field_type = "B_INT32_TYPE";
			field_data << "{ ";			
			for (const int32* data_ptr = (const int32*)rawData;
				rawCount > 0;
				rawCount--, data_ptr++)
			{
				separator = (1 < rawCount) ? ", " : " }";
				field_data << *data_ptr << separator;
			}
			break;
		}
		
		case B_INT64_TYPE:
		{
			field_type = "B_INT64_TYPE";
			field_data << "{ ";			
			for (const int64* data_ptr = (const int64*)rawData;
				rawCount > 0;
				rawCount--, data_ptr++)
			{
				separator = (1 < rawCount) ? ", " : " }";
				field_data << *data_ptr << separator;
			}
			break;
		}
		
		case B_UINT8_TYPE:
		{
			field_type = "B_UINT8_TYPE";
			field_data << "{ ";			
			for (const uint8* data_ptr = (const uint8*)rawData;
				rawCount > 0;
				rawCount--, data_ptr++)
			{
				separator = (1 < rawCount) ? ", " : " }";
				field_data << (uint32)(*data_ptr) << separator;
			}
			break;
		}
		
		case B_UINT16_TYPE:
		{
			field_type = "B_UINT16_TYPE";
			field_data << "{ ";			
			for (const uint16* data_ptr = (const uint16*)rawData;
				rawCount > 0;
				rawCount--, data_ptr++)
			{
				separator = (1 < rawCount) ? ", " : " }";
				field_data << (uint32)(*data_ptr) << separator;
			}
			break;
		}
		
		case B_UINT32_TYPE:
		{
			field_type = "B_UINT32_TYPE";
			field_data << "{ ";			
			for (const uint32* data_ptr = (const uint32*)rawData;
				rawCount > 0;
				rawCount--, data_ptr++)
			{
				separator = (1 < rawCount) ? ", " : " }";
				field_data << *data_ptr << separator;
			}
			break;
		}
		
		case B_UINT64_TYPE:
		{
			field_type = "B_UINT64_TYPE";
			field_data << "{ ";			
			for (const uint64* data_ptr = (const uint64*)rawData;
				rawCount > 0;
				rawCount--, data_ptr++)
			{
				separator = (1 < rawCount) ? ", " : " }";
				field_data << *data_ptr << separator;
			}
			break;
		}
		
		case B_FLOAT_TYPE:
		{
			field_type = "B_FLOAT_TYPE";
			field_data << "{ ";			
			for (const float* data_ptr = (const float*)rawData;
				rawCount > 0;
				rawCount--, data_ptr++)
			{
				separator = (1 < rawCount) ? ", " : " }";
				field_data << *data_ptr << separator;
			}
			break;
		}
		
		case B_DOUBLE_TYPE:
		{
			field_type = "B_DOUBLE_TYPE";
			field_data << "{ ";			
			for (const double* data_ptr = (const double*)rawData;
				rawCount > 0;
				rawCount--, data_ptr++)
			{
				separator = (1 < rawCount) ? ", " : " }";
				sprintf(value_buffer, "%f", *data_ptr);
				field_data << value_buffer << separator;
			}
			break;
		}
		
		case B_BOOL_TYPE:
		{
			field_type = "B_BOOL_TYPE";
			field_data << "{ ";			
			for (const bool* data_ptr = (const bool*)rawData;
				rawCount > 0;
				rawCount--, data_ptr++)
			{
				separator = (1 < rawCount) ? ", " : " }";
				sprintf(value_buffer, "%s", (true == *data_ptr) ? "true" : "false");
				field_data << value_buffer << separator;
			}
			break;
		}
		
		case B_OFF_T_TYPE:
		{
			field_type = "B_OFF_T_TYPE";
			field_data << "{ ";			
			for (const off_t* data_ptr = (const off_t*)rawData;
				rawCount > 0;
				rawCount--, data_ptr++)
			{
				separator = (1 < rawCount) ? ", " : " }";
				field_data << *data_ptr << separator;
			}
			break;
		}
		
		case B_SIZE_T_TYPE:
		{
			field_type = "B_SIZE_T_TYPE";
			field_data << "{ ";			
			for (const size_t* data_ptr = (const size_t*)rawData;
				rawCount > 0;
				rawCount--, data_ptr++)
			{
				separator = (1 < rawCount) ? ", " : " }";
				field_data << *data_ptr << separator;
			}
			break;
		}
		
		case B_SSIZE_T_TYPE:
		{
			field_type = "B_SSIZE_T_TYPE";
			field_data << "{ ";			
			for (const ssize_t* data_ptr = (const ssize_t*)rawData;
				rawCount > 0;
				rawCount--, data_ptr++)
			{
				separator = (1 < rawCount) ? ", " : " }";
				field_data << *data_ptr << separator;
			}
			break;
		}
		
		case B_POINTER_TYPE:
		{
			field_type = "B_POINTER_TYPE";
			break;
		}
		
		case B_OBJECT_TYPE:
		{
			field_type = "B_OBJECT_TYPE";
			break;
		}
		
		case B_MESSAGE_TYPE:
		{
			field_type = "B_MESSAGE_TYPE";
			break;
		}
		
		case B_MESSENGER_TYPE:
		{
			field_type = "B_MESSENGER_TYPE";
			break;
		}
		
		case B_POINT_TYPE:
		{
			field_type = "B_POINT_TYPE";
			field_data << "{ ";			
			for (const BPoint* data_ptr = (const BPoint*)rawData;
				rawCount > 0;
				rawCount--, data_ptr++)
			{
				separator = (1 < rawCount) ? ", " : " }";
				field_data << "(" << data_ptr->x << ", " << data_ptr->y << ")" << separator;
			}
			break;
		}
		
		case B_RECT_TYPE:
		{
			field_type = "B_RECT_TYPE";
			break;
		}
		
//		case B_PATH_TYPE:					s = "B_PATH_TYPE";					break;

		case B_REF_TYPE:
		{
			field_type = "B_REF_TYPE";
			break;
		}
		
		case B_RGB_COLOR_TYPE:
		{
			field_type = "B_RGB_COLOR_TYPE";
			break;
		}
		
		case B_PATTERN_TYPE:
		{
			field_type = "B_PATTERN_TYPE";
			break;
		}
		
		case B_STRING_TYPE:
		{
			field_type = "B_STRING_TYPE";
			field_data << "{ ";			
			for (const char* data_ptr = (const char*)rawData;
				rawCount > 0;
				rawCount--, data_ptr+= strlen(data_ptr) )
			{
				separator = (1 < rawCount) ? ", " : " }";
				field_data << "\"" << data_ptr << "\"" << separator;
			}
			break;
		}
		
		case B_MONOCHROME_1_BIT_TYPE:
		{
			field_type = "B_MONOCHROME_1_BIT_TYPE";
			break;
		}
		
		case B_GRAYSCALE_8_BIT_TYPE:
		{
			field_type = "B_GRAYSCALE_8_BIT_TYPE";
			break;
		}
		
		case B_COLOR_8_BIT_TYPE:
		{
			field_type = "B_COLOR_8_BIT_TYPE";
			break;
		}
		
		case B_RGB_32_BIT_TYPE:
		{
			field_type = "B_RGB_32_BIT_TYPE";
			break;
		}
		
		case B_TIME_TYPE:
		{
			field_type = "B_TIME_TYPE";
			break;
		}
		
		case B_MEDIA_PARAMETER_TYPE:
		{
			field_type = "B_MEDIA_PARAMETER_TYPE";
			break;
		}
		
		case B_MEDIA_PARAMETER_WEB_TYPE:
		{
			field_type = "B_MEDIA_PARAMETER_WEB_TYPE";
			break;
		}
		
		case B_MEDIA_PARAMETER_GROUP_TYPE:
		{
			field_type = "B_MEDIA_PARAMETER_GROUP_TYPE";
			break;
		}
		
		case B_RAW_TYPE:
		{
			field_type = "B_RAW_TYPE";
			break;
		}
		
		case B_MIME_TYPE:
		{
			field_type = "B_MIME_TYPE";
			break;
		}
		
		case B_ANY_TYPE:
		{
			field_type = "B_ANY_TYPE";
			break;
		}
		
		default:
		{
			field_type = "UNKNOWN_TYPE";
			break;
		}
	}

    sprintf(msg_buffer,
            "    %-18s %-18s [%2d] = %s\n",
            field_type,
            fieldName,
            field_count,
            field_data.String() );

    m_outfile->Write(msg_buffer, strlen(msg_buffer) );
}
