#include <PortLink.h>
#include <PortMessage.h>
#include <ServerProtocol.h>
#include <OS.h>
#include "SysCursorAPI.h"

// TODO: tweak the BBitmap and BCursor headers

void set_syscursor(cursor_which which, const BCursor *cursor)
{
/*	port_id server=find_port(SERVER_PORT_NAME);
	if(fServerFrom!=B_NAME_NOT_FOUND)
	{
		PortLink link(server);
		link.SetOpCode(AS_SET_SYSCURSOR_BCURSOR);
		link.Attach<cursor_which>(which);
		link.Attach<int32>(cursor->m_serverToken);
		link.Flush();
	}
*/
}

void set_syscursor(cursor_which which, const BBitmap *bitmap)
{
/*	port_id server=find_port(SERVER_PORT_NAME);
	if(fServerFrom!=B_NAME_NOT_FOUND)
	{
		PortLink link(server);
		link.SetOpCode(AS_SET_SYSCURSOR_BBITMAP);
		link.Attach<cursor_which>(which);
		link.Attach<int32>(cursor->fToken);
		link.Flush();
	}
*/
}

cursor_which get_syscursor(void)
{
	port_id server=find_port(SERVER_PORT_NAME);
	if(server!=B_NAME_NOT_FOUND)
	{
		PortMessage pmsg;
		
		PortLink link(server);
		link.SetOpCode(AS_GET_SYSCURSOR);
		link.FlushWithReply(&pmsg);
		
		cursor_which which;
		pmsg.Read<cursor_which>(&which);
		return which;
	}
	return B_CURSOR_INVALID;
}

void setcursor(cursor_which which)
{
	port_id server=find_port(SERVER_PORT_NAME);
	if(server!=B_NAME_NOT_FOUND)
	{
		PortLink link(server);
		link.SetOpCode(AS_SET_CURSOR_SYSTEM);
		link.Flush();
	}
}
